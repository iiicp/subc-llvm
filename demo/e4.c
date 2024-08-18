int main(){
    int a=0;
    int count=0;
    for(; a<=0; ){
        a=a-1;
        count=count+1;
        if(a<-20)
            break;
    }
    return count;
}