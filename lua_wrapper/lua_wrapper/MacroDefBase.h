#pragma once

//命名空间
#ifndef SHARELIB_BEGIN_NAMESPACE
#define SHARELIB_BEGIN_NAMESPACE namespace shr {
#endif
#ifndef SHARELIB_END_NAMESPACE
#define SHARELIB_END_NAMESPACE }
#endif

//禁用复制
#ifndef SHARELIB_DISABLE_COPY_CLASS
#define SHARELIB_DISABLE_COPY_CLASS(className) \
    className(const className &) = delete;\
    className & operator =(const className &) = delete
#endif
