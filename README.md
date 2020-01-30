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

test3 程序说明
该程序有一个问题， 在创建第二个对象时，定位new运算符使用一个新对象来覆盖用于第一个对象的内存单元。显然，如果类动态地为其成员分配内存， 这将引发问题；
其次， 将delete用于pc2和pc4时， 将自动调用pc2和pc4指向的对象调用析构函数；然后，将delete[]用于buffer时， 不会为使用定位new运算符创建的对象调用析构函数。
***: 程序猿必须负责管用定位new运算符用从中使用的缓冲区内存单元。要使用不同的内存单元，程序猿需要提供两个位于缓冲区的不同地址， 并确保这两个内存单元不重叠 。例如， 可以这样做：
	pc1 = new (buffer) JustTesting;
	pc3 = new (buffer + sizeof(JustTesting)) JustTesting("Better Idea", 6);
另一个问题是： 如果使用定位new 运算符来为对象分配内存， 必须确保其析构函数被调用。但如何确保呢？ 对于在堆中创建的对象， 可以这样做：
	delete pc2;
	但对于定位new运算符分配的对象pc1和pc3， 不能这样做：
	delete pc1;
	delete pc3;
原因在与delete可与常规new运算符配合使用，但不能与定位new运算符配合使用。 比如：指针pc3	没有收到new 运算符返回的地址， 因此delete pc3将导致运行阶段错误。另一方面：指针pc1指向的地址与buffer相同，但buffer是使用new[]初始化的， 因此必须使用delete[]而不是delete来释放。即使buffer是使用new而不是new[]初始化的， delete pc1也将释放buffer，而不是pc1。

那怎么宣布pc1和pc3的死亡呢？
可以显示的调用析构函数。比如：
	pc3->~JustTesting();
	pc1->~JustTesting();
可参见test4.cc
```
