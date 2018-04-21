
# **lua_wrapper**

一个用C++11封装lua的库。

1. 支持任意数量参数；
1. 调用类型支持函数，成员函数，成员变量、**函数对象、Lambda表达式**。
1. 数据类型支持所有枚举，支持const wchar_t*，并且可以**自由扩展自定义类型**。

- [**lua_wrapper**](#luawrapper)
    - [**技术细节:**](#%E6%8A%80%E6%9C%AF%E7%BB%86%E8%8A%82)
    - [**一、Lua背景知识：**](#%E4%B8%80%E3%80%81lua%E8%83%8C%E6%99%AF%E7%9F%A5%E8%AF%86%EF%BC%9A)
        - [**1.1 Lua如何调用C**](#11-lua%E5%A6%82%E4%BD%95%E8%B0%83%E7%94%A8c)
        - [**1.2 给C回调函数设置userdata**](#12-%E7%BB%99c%E5%9B%9E%E8%B0%83%E5%87%BD%E6%95%B0%E8%AE%BE%E7%BD%AEuserdata)
    - [**二、实现C++调用转接到lua的基本方法：**](#%E4%BA%8C%E3%80%81%E5%AE%9E%E7%8E%B0c%E8%B0%83%E7%94%A8%E8%BD%AC%E6%8E%A5%E5%88%B0lua%E7%9A%84%E5%9F%BA%E6%9C%AC%E6%96%B9%E6%B3%95%EF%BC%9A)
    - [**三、需要解决的问题**](#%E4%B8%89%E3%80%81%E9%9C%80%E8%A6%81%E8%A7%A3%E5%86%B3%E7%9A%84%E9%97%AE%E9%A2%98)
        - [**3.1 不想写那么多lua_CFunction，只写一个怎么样？**](#31-%E4%B8%8D%E6%83%B3%E5%86%99%E9%82%A3%E4%B9%88%E5%A4%9Aluacfunction%EF%BC%8C%E5%8F%AA%E5%86%99%E4%B8%80%E4%B8%AA%E6%80%8E%E4%B9%88%E6%A0%B7%EF%BC%9F)
        - [**3.2 函数指针的类型怎么办？**](#32-%E5%87%BD%E6%95%B0%E6%8C%87%E9%92%88%E7%9A%84%E7%B1%BB%E5%9E%8B%E6%80%8E%E4%B9%88%E5%8A%9E%EF%BC%9F)
        - [**3.3 解析函数的参数信息、返回值信息**](#33-%E8%A7%A3%E6%9E%90%E5%87%BD%E6%95%B0%E7%9A%84%E5%8F%82%E6%95%B0%E4%BF%A1%E6%81%AF%E3%80%81%E8%BF%94%E5%9B%9E%E5%80%BC%E4%BF%A1%E6%81%AF)
        - [**3.4 区别调用**](#34-%E5%8C%BA%E5%88%AB%E8%B0%83%E7%94%A8)
        - [**3.5 怎样传参？怎样返回值？**](#35-%E6%80%8E%E6%A0%B7%E4%BC%A0%E5%8F%82%EF%BC%9F%E6%80%8E%E6%A0%B7%E8%BF%94%E5%9B%9E%E5%80%BC%EF%BC%9F)
        - [**3.6 最后要解决的问题**](#36-%E6%9C%80%E5%90%8E%E8%A6%81%E8%A7%A3%E5%86%B3%E7%9A%84%E9%97%AE%E9%A2%98)
        - [**3.7 更进一步**](#37-%E6%9B%B4%E8%BF%9B%E4%B8%80%E6%AD%A5)

## **技术细节:**

为了在项目中对接lua，查找了一些开源库，但都一些不尽如人意的地方：比如，

1. 支持的函数参数数量有限，一旦真遇到超过10参数时，就不支持了；
1. 不支持枚举，不支持自定义类型，不方便扩展等;

因此参照C++11最新的模板技术，自行封装一个改造版的lua库。解决了上面那些问题。

## **一、Lua背景知识：**

### **1.1 Lua如何调用C**

    /*
    ** Type for C functions registered with Lua
    */
    typedef int (*lua_CFunction) (lua_State *L);

lua可以调用的C函数原型如上，lua库会把参数等信息存入`lua_State*`里面，而后调用注册的C函数，在该函数里面实现逻辑，最后把C的返回值push到`lua_State*`的栈上，最后该函数的返回值int表示push了多少个数据到`lua_State*`的栈上。通常写C代码时，一个回调函数总是会给一个额外的`void*`的参数，作为userdata，方便回调时用户使用，注意这里lua的回调没有，它需要通过其它方式解决，下面讲到。 

### **1.2 给C回调函数设置userdata**

要注册C函数到lua里面，最终会调用的一个函数是

    LUA_API void lua_pushcclosure (lua_State *L, lua_CFunction fn, int n);

第一、二个参数容易理解，重点是第三个参数，表示有多少个值被关联到这个注册的函数中。做法是，先push数据到`lus_State*`的栈上，而后调用该函数，把数据的个数传给第三个参数。

这样做了之后，当lua脚本回调到C函数中时，可以用`lua_upvalueindex`取得上面关联到C回调函数中的userdata，比如，关联了3个数据，那么就可以`lua_upvalueindex(1)`, `lua_upvalueindex(2)`, `lua_upvalueindex(3)`取得这三个数据的栈索引，从`lus_State*`的栈上读取该数据。

## **二、实现C++调用转接到lua的基本方法：**

基于上面的lua调用C的基本知识，把C++和lua关联起来，让lua脚本可以调用C++的基本实现逻辑就清楚了，实现一些`lua_CFunction`，从`lua_State*`中读取lua脚本传递的参数，而后把它们转成C++类型，传参，调用合适的C++函数，返回值push到`lus_State*`的栈上，最后返回push的数据个数。

基本思路是清晰的，但实做起来有问题：不同的C++函数，名字，参数，返回值各不相同，这种简单的做法等于每一个转接的C++函数都需要写一个对应的`lua_CFunction`，而且如果C++函数有变动，相应的转接函数也得跟着变，整个操作成本非常大。

我们希望的目标是，能够有一种方法，把C++调用与lua自动关联起来，而后所有的参数传递，返回值等都自动处理好。

## **三、需要解决的问题**

### **3.1 不想写那么多lua_CFunction，只写一个怎么样？**

一个lua_CFunction要对应多种调用，必须有一种机制能够映射到不同的函数，结合二，可以这样实现：把要注册的C++函数指针作为userdata传给公共的注册函数，回调时取出函数指针调用之。

### **3.2 函数指针的类型怎么办？**

借用C++的模板函数特性，把公共的注册函数实现为模板，注册时把函数指针的类型显式实例化注册函数，这样回调过来的时候，可以把函数指针的类型恢复起来。 
下面是代码示例：

    //注册函数指针
    template<class _FuncType>
    void push_cpp_callable_to_lua(lua_State * pLua, _FuncType pf)
    {
        //_FuncType 传值, 自动把函数转成指向函数的指针
        void * ppf = lua_newuserdata(pLua, sizeof(pf));
        assert(ppf);
        //把调用指针写入upvalue
        std::memcpy(ppf, &pf, sizeof(pf));
        ::lua_pushcclosure(pLua, MainLuaCFunctionCall<_FuncType>, 1);
    }

    //这是公共的注册函数
    template<class _FuncType>
    int MainLuaCFunctionCall(lua_State * pLua)
    {
        void * ppf = lua_touserdata(pLua, lua_upvalueindex(1));
        assert(*(_FuncType*)ppf);
        //…
    }

### **3.3 解析函数的参数信息、返回值信息**

这是调用函数的基本要求。这里借鉴stl中的源代码，用一系列的模板特化，把参数及返回值信息解析出来。

    template<class T>
    struct CallableTypeHelper;

    template<class _RetType, class... _ArgType>
    struct CallableTypeHelper<_RetType(* )(_ArgType...)>
    {
        //函数指针的特化
    }

    template<class _RetType, class _ClassType, class... _ArgType>
    struct CallableTypeHelper<_RetType(_ClassType::* )(_ArgType...)>
    { 
        //成员函数指针的特化
    }

    template<class _RetType, class _ClassType>
    struct CallableTypeHelper<_RetType _ClassType::* >
    {
        //成员变量指针的特化
    }

有这些模板特化，只要把公共注册函数`MainLuaCFunctionCall`的模板类型`_FuncType`传给`CallableTypeHelper<_FuncType>`，各种类型便都解析出来了。实际实现的代码还需要考虑一些问题，不同的函数调用类型（`__stdcall、__cdedl、__fastcall、__thiscall`等），`const`属性等，这些可以使用宏来简化代码的编写。
这些类型特性是我们关心的，返回值类型，参数类型，调用类型(函数、成员函数还是成员变量)。这里定义一些类型别名：

    using class_t = …; //如果是成员函数或成员变量，它代表类类型
    using result_t = …; //返回值类型
    using arg_tuple_t = std::tuple<…>; //参数打包而成的tuple类型
    using arg_index_t = typename MakeSequence<…>::type;//参数序列号
    static const CallType call_type = …; //调用类型的常量值

这里的`MakeSequence`作用是把可变参数的模板类型转化为整数序列，实现如下：

    template<size_t ... Index>
    struct IntegerSequence
    {};

    namespace Internal
    {
        template<class T, size_t N>
        struct MakeSequenceHelper;

        template<size_t ... Index>
        struct MakeSequenceHelper<IntegerSequence<Index...>, 0>
        {
            using type = IntegerSequence<Index...>;
        };

        template<size_t ... Index, size_t N>
        struct MakeSequenceHelper<IntegerSequence<Index...>, N>
            : public MakeSequenceHelper<IntegerSequence<N - 1, Index...>, N - 1>
        {};
    }

    //根据可变参数生成参数序列,从0开始
    template<size_t N>
    struct MakeSequence
        : public Internal::MakeSequenceHelper<IntegerSequence<>, N>
    {
    };

`MakeSequence`用可变参数模板实例化时，转到`MakeSequenceHelper`，第一个参数传`IntegerSequence<>`。`MakeSequenceHelper`有一个模板特化，此时优先匹配这个特化的版本，并且可变的类型包被拆分出来，而后，每向下继承一层，便把一个整数值写入到`IntegerSequence`的类型列表中，可变的类型包里便少一个类型。每一次继承，便会再次查找一个最合适的匹配。直到最后可变类型包为0时，只能匹配主版本的`MakeSequenceHelper`，这时定义第一个参数的别名为`type`，它就是`IntegerSequence<0,1,2,…>`.这种技术便是模板元编程。C++14中把这个生成整数序列的算法内置为编译器支持了。

接下来，会看到这些解析出来的类型的作用。

### **3.4 区别调用**

不同的函数指针类型的调用方法是不同的，如函数指针，直接传参；成员函数指针，要用类指针调用；成员变量指针，用类指针调用，不传参。
怎么办？还是模板特化，上面不是已经用模板特化解析出来了各种需要的类型吗，这时候就派上用场了：

    /** 调用分发器
    @Tparam[in] callId: 常量值, 表示调用类型
    @Tparam[in] returnVoid: 常量值, 表示返回类型是否为void
    @Tparam[in] _CallableType: CallableTypeHelper类类型
    @Tparam[in] _IndexType: 参数序列号
    */
    template<CallType callId, bool returnVoid, class _CallableType, class _IndexType>
    struct luaCFunctionDispatcher;

    //模板特化, 函数指针, 返回void
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallType::POINTER_TO_FUNCTION, true, _CallableType, IntegerSequence<index...> >
    {
    };
    //模板特化, 函数指针, 有返回值
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallType::POINTER_TO_FUNCTION, false, _CallableType, IntegerSequence<index...> >
    {
    };
    //模板特化, 成员函数指针, 返回void
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallType::POINTER_TO_MEMBER_FUNCTION, true, _CallableType, IntegerSequence<index...> >
    {
    };
    //模板特化, 成员函数指针, 有返回值
    template<class _CallableType, size_t ... index>
    struct luaCFunctionDispatcher<CallType::POINTER_TO_MEMBER_FUNCTION, false, _CallableType, IntegerSequence<index...> >
    {
    };
    //模板特化, 成员变量指针
    template<class _CallableType>
    struct luaCFunctionDispatcher<CallType::POINTER_TO_MEMBER_DATA, false, _CallableType, IntegerSequence<> >
    {
    };

这里注意模板特化中的常量参数 `size_t ... index`，传类型参数`IntegerSequence<…>`，但是模板的常量参数却不是`IntegerSequence`，这样做的目的是剥离`IntegerSequence`，只保留最有用的整数序列。

### **3.5 怎样传参？怎样返回值？**

上面已经解析出了函数的参数类型，结合参数的整数序列，如果有一个值序列，与函数的参数一一对应，那么只需要按整数序列将可变值序列展开，传参即可。

因此需要制定一个规则，lua调用C++时，必须按C++函数的参数列表，一一对应传参。这样，回调的`lua_State*`中，栈索引`1, 2, …`恰好对应C++函数的参数整数序列。假设有一个辅助函数`from_lua`可以根据栈索引，从lua中读取数据转成C++类型的值，那么就可以按下面的方式调用C++函数：

    pf(from_lua(pLua, index + 1)...);

`pf`代表C++函数指针，另外C++函数的参数整数序列是从0开始，而lua的栈索引是从1开始，因此要把序列号加1.

再假设有一个辅助函数to_lua，可以把C++数据写入到lua中，那么整个的C++调用，并且把C++返回值写回lua就完全可以实现了：

    to_lua(pf(from_lua(pLua, index + 1)...));

有多少个值写入到lua栈上了呢？简单，调用C++之前记录下栈顶，调用之后，返回栈顶增加的数值即可。

至此，整个的注册、回调、转发调用C++，返回值回传lua的流程已经清晰：

- (1) push_cpp_callable_to_lua，将C++调用注册到lua，C++调用指针通过lua的upvalue机制传入，类型通过模板显式实例化实现，所有的C++调用最终归结到一个总的lua回调：MainLuaCFunctionCall

- (2) lua回调C++时，首先进入MainLuaCFunctionCall，而后从upvalue中取出C++调用指针，并且恢复类型，借肋模板特化解析类型，而后根据不同情况区别调用C++指针；

- (3) to_lua(pf(from_lua(pLua, index + 1)...)); 转化lua数据，传参，调用C++函数，返回值回写lua，完成。

### **3.6 最后要解决的问题**

`from_lua`，`to_lua`怎么实现？
Lua要转成C++，需要知道C++类型；同样，C++转lua也需要根据类型。
回顾上面解析函数类型的信息，最终得到了这些类型别名：

    using class_t = …; //如果是成员函数或成员变量，它代表类类型
    using result_t = …; //返回值类型
    using arg_tuple_t = std::tuple<…>; //参数打包而成的tuple类型
    using arg_index_t = typename MakeArgIndex<…>::type;//参数序列号
    static const CallType call_type = …; //调用类型的常量值

其中，`result_t`就是返回值类型，于是`to_lua`可以解决了；`arg_tuple_t`是`tuple`类型，结合整数序列，可以用`std::tuple_element`获取某一个参数的类型，于是`from_lua`也可解决了。不同的C++类型，lua与之转换的做法不相同，要使用一个函数解决这些类型的转换，自然而然地想到，再次使用模板特化，假设该类模板取名为`lua_io_dispatcher`，那么C++函数调用就可以这样实现：

    lua_io_dispatcher<result_t>::to_lua(
        pLua,
        pf(lua_io_dispatcher<
            std::decay_t<std::tuple_element<index, arg_tuple_t>::type >
            >::from_lua(pLua, index + 1)...)
        );

### **3.7 更进一步**

到上面，是目前包括luaplus等开源库的普遍做法，实现上虽千差万别，但基本原理大同小异。不过实际应用的时候，还会遇到问题，就是第6步中，用模板特化实现类型转化这里，普遍的做法就是从`int`，`unsigned int`，`long`，`unsigned long`，`double`，`float`，`…`等基本类型通通特化一遍，看起来支持得也挺全面了，但是，如果遇到用户自定义类型怎么办？C++的函数中使用自定义类型的情况非常普遍，别的不说，单说标准库中，当这些基本类型与`vector`结合时，需要特化的版本数量就得再翻一倍，`list`呢，再翻一倍，`dequeu`，`map`，`set`，`…`，因此，单单通过模板特化是根本无法满足需要的，无论你预先特化多少个版本，都是不够用的。必须让用户能够很方便的进行自由扩展。但模板特化这种做法，对于自由扩展来说，严重不友好。

- (1) 使用模板的地方必须看到所有的特化版本，那些看不到的版本不能生效；

- (2) 所有特化的版本必须和主声明在同一个命名空间中；

第(1)条所导致的后果就是用户为了实现扩展必须把各种头文件都加到该模板实现里，第(2)条所导致的后果就是用户为了实现扩展得修改这个lua封装库。两条都是不可接受的。

为了解决这个问题，可以借鉴C++标准库中IO流的做法，C++的IO流就是一种可以自由扩展自定义类型的实现，只要用户自定义的类型重载了`<<`和`>>`运算符，就可以和标准库完美融合。这就需要在`from_lua`，`to_lua`的实现里，做二次转接，比如定义两个类，`lua_ostream`，`lua_istream`，`from_lua`可以如此实现：

    T from_lua(lua_State * pL, int index)
    {
        T temp{};
        lua_istream is(pL, index);
        is >> temp;
        return temp;
    }

`to_lua`可以如此实现：

    void to_lua(lua_State * pL, const T & value)
    {
        lua_ostream os(pL);
        os << value;
    }

然后，可以仿照C++标准库IO流，预置`lua_ostream`和`lua_istream`对基本类型的重载实现，这样遇到用户自定义的类型时，它只要对该类型重载`<<`和`>>`运算符，就可以自由扩展了。

不过，模板特化也还是有用的，比如，枚举类型，通常他们都可以直接转化为整数，但对C++来说，他们又是一个自定义的类型。如果每个枚举类型再自己去重载，未免累赘。这一点可以使用C++11的`std::is_enum`，`std::underlying_type`来解决：

    template<class T, bool isEnum = std::is_enum<std::decay_t<T>>::value>
    struct lua_io_dispatcher
    {
        //…
    };

而后增加一组模板特化：

    template<class T>
    struct lua_io_dispatcher<T, true>
    {
        using _UType = typename std::underlying_type_t<T>;
        static int to_lua(lua_State * pL, T value)
        {
            return lua_io_dispatcher<_UType>::to_lua(pL, static_cast<_UType>(value));
        }

        static T from_lua(lua_State * pL, int index, T defaultValue = T{})
        {
            lua_stack_check checker(pL);
            return static_cast<T>(lua_io_dispatcher<_UType>::from_lua(
                pL, 
                index, 
                defaultValue));
        }
    };

当类型是枚举时，获取`underlying_type`，通常他就是`int`，之后转接到`int`类型。

至此，才算是真正大功告成。
