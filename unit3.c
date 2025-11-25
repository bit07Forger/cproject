#include<stdio.h>
/*int fact(int n){
if (n==0)
return 1;
else
return n*fact(n-1);
}*/
/*int fib(int n){
    if(n==0 || n==1)
    return n;
    else 
    return fib(n-1)+fib(n-2);
}*/


/*int main (){
int number;
printf("Enter a positive integer: ");   
scanf("%d",&number);
for (int i=0;i<number;i++){
    printf("%d ",fib(i));
}
}*/
int sum(int x,int y);
int main (){
int a=5,b=10;
int result=sum(a,b);

printf("Sum=%d",result);
}
int sum(int x,int y){
    return x+y;
}

   
   

