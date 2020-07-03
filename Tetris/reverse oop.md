# reverse oop

interface
```cpp
class RendererBase
{
    public:
        virtual void draw()...
}
class PCRenderer : public RendererBase
{
    public void draw()...
}
```



```cpp
//header
//선언
class RendererCommon : OS
{
public:
 void draw()...
}
//RendererCommon.cpp는 없음
//RendererPC.cpp
//define
RendererCommon::draw()
{

}
//RendererPC.h
class RendererPC
{

}
/*
플랫폼 전용 데이터 타입: device, context 
*/
#ifdef device(PC)
#define OS RendererPC
#endif
...