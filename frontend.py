import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext, font
import subprocess
import os
import platform
import threading

# --- Configuration ---
# Filenames used by the C++ pipeline execution
LEXER_EXECUTABLE_BASE = "lexical"
LEXER_OUTPUT_FILENAME = "lexer_output.txt" # Lexer output (standard)
SYNTAX_EXE_BASE = "syntax_analyzer"
AST_OUTPUT_FILENAME = "ast_output.txt" # AST output (standard)
ICG_EXE_BASE = "intermediate_gen"
PIPELINE_TAC_OUTPUT_FILENAME = "3ac_output.txt" # ICG writes here
PIPELINE_DAG_VARS_FILENAME = "dag_vars.txt"   # ICG writes here
DAG_EXE_BASE = "dag_builder"
PIPELINE_DAG_OUTPUT_FILENAME = "dag.dot"      # DAG builder writes here

# --- Filenames containing the CORRECT/MANUAL output to DISPLAY ---
DISPLAY_TAC_OUTPUT_FILENAME = "3ac_output1.txt"
DISPLAY_DAG_VARS_FILENAME = "dag_vars1.txt" # (Needed if DAG builder reads it)
DISPLAY_DAG_OUTPUT_FILENAME = "dag1.dot"

# Determine executable names based on OS
# ... (Executable name logic - same as before) ...
if platform.system() == "Windows":
    LEXER_EXECUTABLE = f"{LEXER_EXECUTABLE_BASE}.exe"; SYNTAX_EXECUTABLE = f"{SYNTAX_EXE_BASE}.exe"
    ICG_EXECUTABLE = f"{ICG_EXE_BASE}.exe"; DAG_EXECUTABLE = f"{DAG_EXE_BASE}.exe"
else:
    LEXER_EXECUTABLE = f"./{LEXER_EXECUTABLE_BASE}"; SYNTAX_EXECUTABLE = f"./{SYNTAX_EXE_BASE}"
    ICG_EXECUTABLE = f"./{ICG_EXE_BASE}"; DAG_EXECUTABLE = f"./{DAG_EXE_BASE}"


# --- GUI Application ---
class FullCompilerSimApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("C++ Compiler Sim (Displaying Corrected Files)")
        self.geometry("1100x900")
        # --- Init Code (Executable checks, style, widgets) ---
        # (SAME AS PREVIOUS VERSION - No changes needed here)
        # ... (ensure all the __init__ code from previous correct versions is here) ...
        self.script_dir = os.path.dirname(__file__) # Store script directory
        self.lexer_path = os.path.join(self.script_dir, LEXER_EXECUTABLE.replace("./", ""))
        self.syntax_path = os.path.join(self.script_dir, SYNTAX_EXECUTABLE.replace("./", ""))
        self.icg_path = os.path.join(self.script_dir, ICG_EXECUTABLE.replace("./", ""))
        self.dag_path = os.path.join(self.script_dir, DAG_EXECUTABLE.replace("./", ""))
        self.paths_to_check = { "Lexer": self.lexer_path, "Syntax Analyzer": self.syntax_path, "Intermediate Gen": self.icg_path, "DAG Builder": self.dag_path }
        self.error_msg_startup = ""
        for name, path in self.paths_to_check.items():
            if not os.path.exists(path): self.error_msg_startup += f"- {name} executable ('{os.path.basename(path)}') not found.\n"
        self.style = ttk.Style(self); available_themes = self.style.theme_names()
        try: # Theme setting
            current_themes = list(available_themes); preferred_themes = ['clam', 'alt', 'default']
            if platform.system() == "Windows": preferred_themes.insert(0, 'vista')
            if platform.system() == "Darwin": preferred_themes.insert(0, 'aqua')
            set_theme = None;
            for theme in preferred_themes:
                if theme in current_themes: set_theme = theme; break
            if set_theme: self.style.theme_use(set_theme)
            elif current_themes: self.style.theme_use(current_themes[0])
        except tk.TclError: print("Warning: Could not set initial theme."); pass
        self.heading_font = font.Font(family="Helvetica", size=11, weight="bold"); self.label_font = font.Font(family="Helvetica", size=10)
        self.status_font = font.Font(family="Helvetica", size=9)
        try: self.code_font = font.Font(family="Consolas", size=9); self.code_font.metrics()
        except tk.TclError: self.code_font = font.Font(family="Courier New", size=9)
        # Layout Frames
        self.top_frame = ttk.Frame(self, padding="10"); self.top_frame.pack(side=tk.TOP, fill=tk.X)
        self.results_notebook = ttk.Notebook(self, padding="10"); self.results_notebook.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        self.lexer_frame = ttk.Frame(self.results_notebook, padding=5); self.ast_frame = ttk.Frame(self.results_notebook, padding=5)
        self.tac_frame = ttk.Frame(self.results_notebook, padding=5); self.dag_frame = ttk.Frame(self.results_notebook, padding=5)
        self.results_notebook.add(self.lexer_frame, text=' Lexer Tokens '); self.results_notebook.add(self.ast_frame, text=' AST (Sim) ')
        self.results_notebook.add(self.tac_frame, text=' 3AC (Corrected) '); self.results_notebook.add(self.dag_frame, text=' DAG (Corrected .dot) ') # Updated Titles
        self.bottom_frame = ttk.Frame(self, padding=(10, 5, 10, 5)); self.bottom_frame.pack(side=tk.BOTTOM, fill=tk.X)
        self.status_frame = ttk.Frame(self, relief=tk.SUNKEN, padding=(5, 2)); self.status_frame.pack(side=tk.BOTTOM, fill=tk.X)
        # Top Widgets
        self.run_button = ttk.Button(self.top_frame, text="Select C++ File & Analyze", command=self.select_and_run_threaded); self.run_button.grid(row=0, column=0, padx=(0, 10), pady=5)
        if self.error_msg_startup: self.run_button.config(state=tk.DISABLED)
        self.clear_button = ttk.Button(self.top_frame, text="Clear All", command=self.clear_all); self.clear_button.grid(row=0, column=1, padx=(0, 10), pady=5)
        self.file_label = ttk.Label(self.top_frame, text="No C++ file selected.", font=self.label_font, width=60, anchor='w'); self.file_label.grid(row=0, column=2, sticky="ew", padx=(0, 10), pady=5)
        ttk.Label(self.top_frame, text="Theme:", font=self.label_font).grid(row=0, column=3, padx=(10, 5), pady=5)
        self.theme_combobox = ttk.Combobox(self.top_frame, values=available_themes, state='readonly', width=10); self.theme_combobox.grid(row=0, column=4, pady=5)
        try: self.theme_combobox.set(self.style.theme_use())
        except tk.TclError: self.theme_combobox.set(available_themes[0])
        self.theme_combobox.bind("<<ComboboxSelected>>", self.apply_theme); self.top_frame.columnconfigure(2, weight=1)
        # Result Widgets
        ttk.Label(self.lexer_frame, text="Lexical Analysis Tokens:", font=self.heading_font).pack(anchor='w', pady=(0, 5))
        lexer_tree_container = ttk.Frame(self.lexer_frame); lexer_tree_container.pack(fill=tk.BOTH, expand=True)
        lex_scroll_y = ttk.Scrollbar(lexer_tree_container, orient=tk.VERTICAL); lex_scroll_x = ttk.Scrollbar(lexer_tree_container, orient=tk.HORIZONTAL)
        self.token_tree = ttk.Treeview( lexer_tree_container, columns=("Num", "Type", "Lexeme"), show="headings", yscrollcommand=lex_scroll_y.set, xscrollcommand=lex_scroll_x.set )
        lex_scroll_y.pack(side=tk.RIGHT, fill=tk.Y); lex_scroll_x.pack(side=tk.BOTTOM, fill=tk.X); self.token_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        lex_scroll_y.config(command=self.token_tree.yview); lex_scroll_x.config(command=self.token_tree.xview)
        self.token_tree.heading("Num", text="Token Num"); self.token_tree.heading("Type", text="Type"); self.token_tree.heading("Lexeme", text="Lexeme")
        self.token_tree.column("Num", width=80, anchor=tk.W, stretch=tk.NO); self.token_tree.column("Type", width=120, anchor=tk.W, stretch=tk.NO); self.token_tree.column("Lexeme", width=400, anchor=tk.W)
        self.style.configure("Treeview", font=self.code_font, rowheight=int(self.code_font.metrics("linespace")*1.2)); self.style.configure("Treeview.Heading", font=self.heading_font)
        try: self.token_tree.tag_configure('oddrow', background=self.style.lookup('TEntry', 'fieldbackground')); self.token_tree.tag_configure('evenrow', background=self.style.lookup('TEntry', 'background'))
        except tk.TclError: self.token_tree.tag_configure('oddrow', background='#EFEFEF'); self.token_tree.tag_configure('evenrow', background='white')
        ttk.Label(self.ast_frame, text="Syntax Analysis AST (Simplified Simulation):", font=self.heading_font).pack(anchor='w', pady=(0, 5))
        self.ast_text = scrolledtext.ScrolledText(self.ast_frame, height=10, wrap=tk.WORD, font=self.code_font, state=tk.DISABLED, relief=tk.SOLID, borderwidth=1); self.ast_text.pack(fill=tk.BOTH, expand=True)
        ttk.Label(self.tac_frame, text="Three-Address Code (Corrected - from file):", font=self.heading_font).pack(anchor='w', pady=(0, 5)) # Updated label
        self.tac_text = scrolledtext.ScrolledText(self.tac_frame, height=10, wrap=tk.WORD, font=self.code_font, state=tk.DISABLED, relief=tk.SOLID, borderwidth=1); self.tac_text.pack(fill=tk.BOTH, expand=True)
        ttk.Label(self.dag_frame, text="DAG (Corrected .dot - from file):", font=self.heading_font).pack(anchor='w', pady=(0, 5)) # Updated label
        self.dag_text = scrolledtext.ScrolledText(self.dag_frame, height=10, wrap=tk.NONE, font=self.code_font, state=tk.DISABLED, relief=tk.SOLID, borderwidth=1); self.dag_text.pack(fill=tk.BOTH, expand=True)
        ttk.Label(self.bottom_frame, text="Compiler Messages/Errors:", font=self.label_font).pack(anchor='w')
        self.error_text = scrolledtext.ScrolledText(self.bottom_frame, height=6, wrap=tk.WORD, font=self.code_font, state=tk.DISABLED, relief=tk.SOLID, borderwidth=1); self.error_text.pack(fill=tk.X, expand=True)
        self.status_label = ttk.Label(self.status_frame, text="Ready", anchor='w', font=self.status_font); self.status_label.pack(fill=tk.X)
        # --- Show startup errors ---
        if self.error_msg_startup: self.append_error_text("--- Startup Errors ---"); self.append_error_text(self.error_msg_startup.strip()); self.append_error_text("Please ensure C++ programs are compiled."); self.update_status("Error: Missing required C++ executables!")
        else: self.update_status("Ready - All executables found.")

    # --- Methods ---
    # ... (apply_theme, update_status, clear*, append_error_text - SAME AS PREVIOUS CORRECT VERSION) ...
    def apply_theme(self, event=None): # --- (Corrected version) ---
        selected_theme = self.theme_combobox.get(); tcl_error = False
        try: self.style.theme_use(selected_theme)
        except tk.TclError: messagebox.showerror("Theme Error", f"Could not apply theme '{selected_theme}'."); tcl_error = True
        if not tcl_error:
            try: self.token_tree.tag_configure('oddrow', background=self.style.lookup('TEntry', 'fieldbackground')); self.token_tree.tag_configure('evenrow', background=self.style.lookup('TEntry', 'background'))
            except tk.TclError: self.token_tree.tag_configure('oddrow', background='#EFEFEF'); self.token_tree.tag_configure('evenrow', background='white')
            self.update_status(f"Theme changed to '{selected_theme}'")
        if tcl_error: 
            try: self.theme_combobox.set(self.style.theme_use()) 
            except tk.TclError: pass
    def update_status(self, message): self.status_label.config(text=message); self.update_idletasks()
    def clear_all(self): self.clear_tree(); self.clear_ast_text(); self.clear_tac_text(); self.clear_dag_text(); self.clear_error_text(); self.file_label.config(text="No C++ file selected."); self.update_status("Results cleared. Ready.")
    def clear_tree(self): self.token_tree.delete(*self.token_tree.get_children())
    def clear_ast_text(self): self.ast_text.config(state=tk.NORMAL); self.ast_text.delete('1.0', tk.END); self.ast_text.config(state=tk.DISABLED)
    def clear_tac_text(self): self.tac_text.config(state=tk.NORMAL); self.tac_text.delete('1.0', tk.END); self.tac_text.config(state=tk.DISABLED)
    def clear_dag_text(self): self.dag_text.config(state=tk.NORMAL); self.dag_text.delete('1.0', tk.END); self.dag_text.config(state=tk.DISABLED)
    def clear_error_text(self): self.error_text.config(state=tk.NORMAL); self.error_text.delete('1.0', tk.END); self.error_text.config(state=tk.DISABLED)
    def append_error_text(self, message): self.error_text.config(state=tk.NORMAL); self.error_text.insert(tk.END, message + "\n"); self.error_text.see(tk.END); self.error_text.config(state=tk.DISABLED)


    def select_and_run_threaded(self):
        # --- Executable check (same) ---
        current_errors = ""; missing_exec = False
        for name, path in self.paths_to_check.items():
            if not os.path.exists(path): current_errors += f"- {name} executable ('{os.path.basename(path)}') not found.\n"; missing_exec = True
        if missing_exec: messagebox.showwarning("Missing Executables","Cannot run analysis:\n\n" + current_errors + "\nPlease compile."); self.update_status("Error: Missing executables."); self.clear_error_text(); self.append_error_text("--- Cannot Run ---"); self.append_error_text(current_errors.strip()); return

        cpp_filepath = filedialog.askopenfilename(title="Select C++ File", filetypes=(("C++", "*.cpp"), ("C", "*.c"), ("H", "*.h"), ("All", "*.*")))
        if not cpp_filepath: self.update_status("File selection cancelled."); return

        # --- Define paths for PIPELINE files ---
        self.abs_lexer_out = os.path.join(self.script_dir, LEXER_OUTPUT_FILENAME)
        self.abs_ast_out = os.path.join(self.script_dir, AST_OUTPUT_FILENAME)
        self.abs_pipeline_tac_out = os.path.join(self.script_dir, PIPELINE_TAC_OUTPUT_FILENAME) # 3ac_output.txt
        self.abs_pipeline_dag_vars = os.path.join(self.script_dir, PIPELINE_DAG_VARS_FILENAME) # dag_vars.txt
        self.abs_pipeline_dag_out = os.path.join(self.script_dir, PIPELINE_DAG_OUTPUT_FILENAME) # dag.dot

        # --- Define paths for files to DISPLAY ---
        self.abs_display_tac_out = os.path.join(self.script_dir, DISPLAY_TAC_OUTPUT_FILENAME) # 3ac_output1.txt
        self.abs_display_dag_out = os.path.join(self.script_dir, DISPLAY_DAG_OUTPUT_FILENAME) # dag1.dot

        # --- Check if DISPLAY files exist ---
        display_files_missing = False
        if not os.path.exists(self.abs_display_tac_out):
            self.append_error_text(f"Warning: Display file '{DISPLAY_TAC_OUTPUT_FILENAME}' not found.")
            display_files_missing = True
        if not os.path.exists(self.abs_display_dag_out):
            self.append_error_text(f"Warning: Display file '{DISPLAY_DAG_OUTPUT_FILENAME}' not found.")
            display_files_missing = True
        # Optional: Show warning if display files are missing before running pipeline
        # if display_files_missing:
        #    messagebox.showwarning("Missing Display Files", "One or more '...1' files containing the output to display are missing.")

        # --- Cleanup PIPELINE Output Files ---
        # Only clean the files the C++ pipeline ACTUALLY writes to
        files_to_clean = [self.abs_lexer_out, self.abs_ast_out, self.abs_pipeline_tac_out, self.abs_pipeline_dag_vars, self.abs_pipeline_dag_out]
        for f_path in files_to_clean:
            try:
                if os.path.exists(f_path): os.remove(f_path)
            except OSError as e: self.append_error_text(f"Warning: Could not delete '{os.path.basename(f_path)}': {e}")

        # --- Start analysis (same) ---
        self.run_button.config(state=tk.DISABLED); self.clear_button.config(state=tk.DISABLED)
        self.clear_all(); self.file_label.config(text=f"Selected: {os.path.basename(cpp_filepath)}")
        self.update_status(f"Starting analysis pipeline for {os.path.basename(cpp_filepath)}...")
        self.append_error_text(f"--- Starting Pipeline for {os.path.basename(cpp_filepath)} ---")
        thread = threading.Thread(target=self.run_analysis_pipeline, args=(cpp_filepath,), daemon=True); thread.start()

    def run_analysis_pipeline(self, cpp_filepath):
        """Runs C++ programs (writing to standard files), reads display data from specific files."""
        # Store results read from actual pipeline output files
        pipeline_results = {'lexer': None, 'ast': None, 'tac_pipeline': None, 'dag_pipeline': None}
        # Store results read from the specific display files
        display_results = {'tac_display': None, 'dag_display': None}
        errors = []
        current_stage = "Pipeline Start"
        pipeline_ok = True

        # --- Define stages for PIPELINE execution ---
        # C++ programs write to standard filenames
        # DAG builder is CALLED with paths to standard filenames
        stages = [
            { "name": "Lexer", "cmd": [self.lexer_path, cpp_filepath], "in_files": [], "out_files": [self.abs_lexer_out], "result_key": "lexer" },
            { "name": "Syntax Analyzer", "cmd": [self.syntax_path, self.abs_lexer_out], "in_files": [self.abs_lexer_out], "out_files": [self.abs_ast_out], "result_key": "ast" },
            { # ICG writes to PIPELINE files
              "name": "Intermediate Code Gen", "cmd": [self.icg_path, self.abs_lexer_out], "in_files": [self.abs_lexer_out], "out_files": [self.abs_pipeline_tac_out, self.abs_pipeline_dag_vars], "result_key": "tac_pipeline" },
            { # DAG builder reads PIPELINE files, writes PIPELINE file
              "name": "DAG Builder", "cmd": [self.dag_path, self.abs_pipeline_tac_out, self.abs_pipeline_dag_vars], "in_files": [self.abs_pipeline_tac_out, self.abs_pipeline_dag_vars], "out_files": [self.abs_pipeline_dag_out], "result_key": "dag_pipeline" }
        ]

        # --- Run the Pipeline ---
        try:
            for stage in stages:
                current_stage = stage["name"]
                self.update_status(f"Running {current_stage}...")
                # Input file check
                missing_inputs = [os.path.basename(f) for f in stage["in_files"] if not os.path.exists(f)]
                if missing_inputs: errors.append(f"Input file(s) for '{current_stage}' not found: {', '.join(missing_inputs)}."); pipeline_ok = False; break
                # Execute
                proc = subprocess.run(stage["cmd"], capture_output=True, text=True, check=False, encoding='utf-8', errors='ignore', cwd=self.script_dir)
                if proc.stdout: errors.append(f"--- {current_stage} Output ---\n{proc.stdout.strip()}")
                if proc.stderr: errors.append(f"--- {current_stage} Errors ---\n{proc.stderr.strip()}")
                if proc.returncode != 0: errors.append(f"\nError: {current_stage} failed (Exit Code: {proc.returncode})"); pipeline_ok = False; break
                # Output file check (check existence of files pipeline *should* create)
                missing_outputs = [os.path.basename(f) for f in stage["out_files"] if not os.path.exists(f)]
                if missing_outputs: errors.append(f"\nError: Pipeline output file(s) not found after {current_stage}: {', '.join(missing_outputs)}"); pipeline_ok = False; break

                # Read result from the primary pipeline output file for lexer/ast
                if stage["result_key"] == "lexer" or stage["result_key"] == "ast":
                    pipeline_output_file = stage["out_files"][0]
                    try:
                        with open(pipeline_output_file, 'r', encoding='utf-8') as f:
                            pipeline_results[stage["result_key"]] = f.read()
                    except Exception as e:
                        errors.append(f"\nError reading {current_stage} output '{os.path.basename(pipeline_output_file)}': {e}")
                        # Don't necessarily stop pipeline if reading fails

            final_status = f"Pipeline finished for: {os.path.basename(cpp_filepath)}"
            if not pipeline_ok: final_status += f" (with errors during '{current_stage}')"

        except FileNotFoundError as e: errors.append(f"Fatal Error: Executable not found: {e.filename}"); final_status = f"Error: Executable missing."; pipeline_ok = False
        except Exception as e: errors.append(f"Python error during {current_stage}: {e}"); final_status = f"Pipeline coordination error."; pipeline_ok = False

        # --- AFTER PIPELINE: Read the specific DISPLAY files ---
        try:
            with open(self.abs_display_tac_out, 'r', encoding='utf-8') as f:
                display_results['tac_display'] = f.read()
        except Exception as e:
            errors.append(f"\nError reading DISPLAY file '{DISPLAY_TAC_OUTPUT_FILENAME}': {e}")
            display_results['tac_display'] = f"ERROR READING {DISPLAY_TAC_OUTPUT_FILENAME}" # Show error in display

        try:
            with open(self.abs_display_dag_out, 'r', encoding='utf-8') as f:
                display_results['dag_display'] = f.read()
        except Exception as e:
            errors.append(f"\nError reading DISPLAY file '{DISPLAY_DAG_OUTPUT_FILENAME}': {e}")
            display_results['dag_display'] = f"ERROR READING {DISPLAY_DAG_OUTPUT_FILENAME}" # Show error in display


        errors = [msg for msg in errors if msg and not msg.isspace() and ("---" not in msg or len(msg.splitlines()) > 1)]
        # Pass BOTH results dicts to the GUI update function
        self.after(0, self.update_gui_after_pipeline, pipeline_results, display_results, errors, final_status)

    # *** MODIFIED FUNCTION SIGNATURE & LOGIC ***
    def update_gui_after_pipeline(self, pipeline_results, display_results, errors, final_status):
        """Updates GUI. Displays pipeline Lexer/AST, but Corrected 3AC/DAG from specific files."""
        self.run_button.config(state=tk.NORMAL); self.clear_button.config(state=tk.NORMAL)
        self.append_error_text(f"--- Pipeline Execution Finished ---")
        if errors:
            for msg in errors: self.append_error_text(msg)
        else: self.append_error_text("Pipeline ran (check C++ program output/errors above).")

        # --- Display results ---
        # Display actual pipeline Lexer/AST output
        self.display_lexer_results(pipeline_results.get('lexer'))
        self.display_ast_results(pipeline_results.get('ast'))

        # *** Use results read from the DISPLAY files ***
        self.display_tac_results(display_results.get('tac_display'))
        self.display_dag_results(display_results.get('dag_display'))
        # *********************************************

        self.update_status(final_status + " (Displaying Corrected 3AC/DAG)") # Update status

        # List the ACTUAL files generated by the pipeline AND the files displayed
        self.append_error_text(f"Pipeline generated:")
        self.append_error_text(f"- {LEXER_OUTPUT_FILENAME}")
        self.append_error_text(f"- {AST_OUTPUT_FILENAME}")
        self.append_error_text(f"- {PIPELINE_TAC_OUTPUT_FILENAME}") # The standard 3AC file
        self.append_error_text(f"- {PIPELINE_DAG_VARS_FILENAME}")  # The standard Vars file
        self.append_error_text(f"- {PIPELINE_DAG_OUTPUT_FILENAME}") # The standard DAG file
        self.append_error_text(f"Displaying content from:")
        self.append_error_text(f"- {DISPLAY_TAC_OUTPUT_FILENAME}") # The ...1 3AC file
        self.append_error_text(f"- {DISPLAY_DAG_OUTPUT_FILENAME}") # The ...1 DAG file


    # --- display_* methods (No changes needed here) ---
    # ... (Keep the display methods exactly as they were in the previous correct version) ...
    def display_lexer_results(self, file_content): # ... (same as before) ...
        self.clear_tree(); row_index = 0
        if not file_content: self.token_tree.insert("", tk.END, values=("", "(No Lexer Output)", "")); return
        lines = file_content.splitlines()
        if len(lines) < 3: self.token_tree.insert("", tk.END, values=("", "(Malformed Lexer Output)", "")); return
        for line in lines[2:]:
            line = line.strip(); tag = 'evenrow' if row_index % 2 == 0 else 'oddrow'
            if not line: continue
            parts = line.split(" | ", 2)
            if len(parts) == 3: self.token_tree.insert("", tk.END, values=(parts[0].strip(), parts[1].strip(), parts[2].strip()), tags=(tag,))
            else: self.token_tree.insert("", tk.END, values=("?", "Parse Error", line), tags=(tag,))
            row_index += 1
        if row_index == 0: self.token_tree.insert("", tk.END, values=("", "(No Tokens Found)", ""))
    def display_ast_results(self, file_content): # ... (same as before) ...
        self.clear_ast_text(); self.ast_text.config(state=tk.NORMAL)
        self.ast_text.insert('1.0', file_content if file_content else "(No AST generated or file empty)")
        self.ast_text.config(state=tk.DISABLED)
    def display_tac_results(self, file_content): # ... (same as before) ...
        self.clear_tac_text(); self.tac_text.config(state=tk.NORMAL)
        self.tac_text.insert('1.0', file_content if file_content else "(No 3AC content found in display file)")
        self.tac_text.config(state=tk.DISABLED)
    def display_dag_results(self, file_content): # ... (same as before) ...
        self.clear_dag_text(); self.dag_text.config(state=tk.NORMAL)
        if file_content:
            self.dag_text.insert('1.0', file_content)
            self.dag_text.insert(tk.END, "\n\n# --- End of DOT Source ---")
            self.dag_text.insert(tk.END, "\n# Copy this content and use a Graphviz tool to visualize the DAG.")
        else: self.dag_text.insert('1.0', "(No DAG .dot content found in display file)")
        self.dag_text.config(state=tk.DISABLED)


# --- Main Execution --- (No changes needed here) ---
if __name__ == "__main__":
    # ... (Executable check logic - same as before) ...
    paths_to_check_main = { "Lexer": os.path.join(os.path.dirname(__file__), LEXER_EXECUTABLE.replace("./", "")), "Syntax Analyzer": os.path.join(os.path.dirname(__file__), SYNTAX_EXECUTABLE.replace("./", "")), "Intermediate Gen": os.path.join(os.path.dirname(__file__), ICG_EXECUTABLE.replace("./", "")), "DAG Builder": os.path.join(os.path.dirname(__file__), DAG_EXECUTABLE.replace("./", "")) }
    error_msg_main = ""; missing_exec = False
    for name, path in paths_to_check_main.items():
        if not os.path.exists(path): error_msg_main += f"- {name} executable ('{os.path.basename(path)}') not found.\n"; missing_exec = True
    if missing_exec: root = tk.Tk(); root.withdraw(); messagebox.showerror("Fatal Error - Missing Executable(s)", "Could not find:\n\n" + error_msg_main + f"\nPlease compile the .cpp files and place executables in the script directory.\n\nApplication will exit."); root.destroy()
    else: app = FullCompilerSimApp(); app.mainloop()