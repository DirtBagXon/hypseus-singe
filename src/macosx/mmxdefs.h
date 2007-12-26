/*
 *  mmxdefs.h
 *  Daphne
 *
 *  Created by Derek S on 10/8/07.
 *  Copyright 2007 . All rights reserved.
 *
 */
 
 // Ensure all the x86 defines and optimizations exist on OSX
 #ifdef MAC_OSX
 #ifdef __i386__
 #define USE_MMX
 #define ARCH_X86
 #define NATIVE_CPU_X86
 #include <mmintrin.h>
 #endif //i386
 #endif //MAC_OSX

