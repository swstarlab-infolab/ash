#ifndef GSTREAM_CONSTANT_H
#define GSTREAM_CONSTANT_H

namespace gstream {

enum class gstream_error_code : unsigned int {
    Success,
    CudaError,
    RuntimeError,
    LogicError,
    InvalidCall,
    InvalidArgument,
    BadAlloc,
    BadFree,
    CppStdException,
    CppStdUnknownException,
    BufferOverflow,
    FileNotFound,
    FileOpenError,
    FileReadError,
    FileWriteError,
    InitFailure,
    ChannelBroken,
    DataLoadingError,
};

} // !namespace gstream

#endif // GSTREAM_CONSTANT_H
