#include<bits/stdc++.h>
using namespace std;
int factorial(int num){
    if(num<=1)
        return 1;
    return num*factorial(num-1);
}
int main(){
    int a , b;
    cin >> a >> b;
    int c=factorial(a)*factorial(b);
    cout<<c;
}