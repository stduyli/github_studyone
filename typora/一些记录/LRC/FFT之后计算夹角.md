```
CosB = (x1*x2+y1*y2)/(sqrt(x1*x1+y1*y1)*sqrt(x2*x2+y2*y2));        //求向量夹角的坐标运算 x/y分别为FFT结果的实部/虚部数据
Phase = acos(CosB)*57.29578;        //角度换算
if((x1*y2-x2*y1) < 0)                //判断向量夹角方向的坐标运算，0-π Sin符号为正
        Phase = -Phase;
```

