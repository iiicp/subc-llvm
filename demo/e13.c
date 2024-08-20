int sum(int array[ ][4],int m,int n);//该函数完成对array数组中的前m行和n列元素求和

int main()
{
    //定义二维数组的同时进行初始化
    int a[4][4]= {{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}}; 
    int row = 3, col = 2;
    //输出二维数组前row行前col列的元素的和
    return sum(a, row, col); 
}

int sum(int array[][4], int m, int n) {
    int s = 0;
    int i, j;
    for (i = 0; i < m; ++i) {
        for (j = 0; j < n; ++j) {
            s += array[i][j];
        }
    }
    return s;
}