//
//  fbxptr.hpp
//  fbxconcat
//
//  Created by LARRYHOU on 2021/3/5.
//  Copyright © 2021 LARRYHOU. All rights reserved.
//

#ifndef fbxptr_hpp
#define fbxptr_hpp

template<typename T>
class FbxPtr
{
    static int __count;
    T *__obj;
    
public:
    FbxPtr(): __obj(T::Create()) { ++__count; }
    FbxPtr(T *o): __obj(o) { ++__count; }
    
    ~FbxPtr()
    {
        if (--__count == 0) { __obj->Destroy(); }
        else if (__count < 0) { __count = 0; }
    }
    
    T& operator*() { return *__obj; }
    T* operator->() { return __obj; }
    operator T*() { return __obj; }
};

template<typename T>
int FbxPtr<T>::__count = 0;


#endif /* fbxptr_hpp */
