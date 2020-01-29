```
什么是placement new 操作？
c++的new操作符会创建一个对象， 它完成两部操作：
1. 分配对象内存
2. 调用对象类的构造函数创建对象

通常分配的内存是在堆中
但是有些场景下， 我们预先分配了内存， 想要在已知的内存上创建对象怎么办呢？ 
placement new 就是实现这个目的的。 其语法是：
 Object* p = new (address) ClassConstruct(...)

应用场景： 在进程间使用共享内存的时候， C++的placement new经常被用到。例如主进程分配共享内容，然后在共享内存上创建C++类对象，然后从进程直接attach到这块共享内容。拿到类对象， 直接访问类对象的变量和函数。

这里以一个例子来说明：
1. 主进程以server的方式启动
	· 分配共享内存
	· 在共享内存上通过placement new 创建SHMObj
2. 子进程以普通方式启动
	· attach到主进程的共享内存
	· 拿到代表SHMObj对象的指针

运行
主进程：
$ ./main server
CMD> attach  
succ: shmget and shmat for key:3887 shm_id:1736729 address:0x7f3659c26000 object:0
CMD> create
SUCC: create(1), address=[0x59c26000],object=[0x59c26000]
CMD> print
count = 100
CMD> increase
count = 101
CMD> increase
count = 102
CMD> increase
count = 103
CMD> print
count = 103

从进程：
$ ./main
CMD> attach
succ: shmget and shmat for key:3887 shm_id:1736729 address:0x7f8cc66d6000 object:0
CMD> create
SUCC: create(0), address=[0xc66d6000],object=[0xc66d6000]
CMD> print
count = 103

从这个例子我们看到对象shkm_object在主进程里面被创建(placement new)， 但是在从进程里面并没有被创建， 而是直接从共享内存里面解析出来， 然后直接访问类成员和函数

* 需要注意的是， 创建出来的对象的地址就是共享内存的地址，就是基于这个属性，我们的功能才能被实现。也就是说：
Object* p = new (address) ClassConstruct(...)
返回p的值， 和输入地址address的值是相同的。

test2 程序说明
1. 针对定位new运算符分配的内存块， 不需要跟踪哪些内存单元已被使用， 也不用查找未使用的内存块。这些工作全都交给程序猿
2. 针对常规的new运算符， 需要用delete来释放内存，但对于定位new运算符， 不需要用new来释放内存，因为有时候用delete释放new定位运算符的内存是错误的。
3. 定位new运算符的另外一种用法：可以与初始化结合使用，从而将信息放在特定的硬件地址处。

定位运算符的其他形式：
int * p1 = new int; // ivokes new(sizeof(int))
int * p2 = new(buffer) int; // ivokes new(sizeof(int), buffer)
int * p3 = new(buffer) int[40]; // ivokes new(40*sizeof(int), buffer)
定位运算符new函数不可替换， 但是可以重载， 它至少需要接收两个参数。其中第一个总是std::size_t, 指定了请求的字节数。

```
