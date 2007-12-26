# MMX RGB2YUV conversion function
# by Matt Ownby
# For GAS (GNU Assembler)

# This function is equivalent to this C code from Intel:
#
#       Y = (unsigned char) (((9798*R)+(19235*G)+(3736*B))/32768);
#       U = (unsigned char) ((((-5537*R)+(-10878*G)+(16384*B))/32768) + 128);
#       V = (unsigned char) ((((16384*R)+(-13729*G)+(-2664*B))/32768) + 128);

################################################################################

# C function prototype is:

#       extern "C" void asm_rgb2yuv();

# This function is controlled through global variables (because I found that to be the fastest).
# To set the RGB values that you want converted, you write three 8-bit values (R, G, and B) to
# an array of 16-bit shorts (I found this to be the fastest).
# As far as you're concerned, the array you write to looks like this:
#
#       extern "C" unsigned short asm_rgb2yuv_input[3];
#
# (it actually is 64-bits but the last element is ignored)
#
# Here's some example C code to set your input:
#
#       unsigned char rgb[3] = { 255, 0, 0 };   // a red color for example (notice it's 8-bit!!)
#       asm_rgb2yuv_input[0] = rgb[0];
#       asm_rgb2yuv_input[1] = rgb[1];
#       asm_rgb2yuv_input[2] = rgb[2];
#       asm_rgb2yuv();  // do the calculation here
#
# OK after you've done your calculation, the 8-bit YUV results will be written to three global
# variables:
#
#       extern "C" unsigned char asm_rgb2yuv_result_y;
#       extern "C" unsigned char asm_rgb2yuv_result_u;
#       extern "C" unsigned char asm_rgb2yuv_result_v;
#
#       NOTE : these are actually 64-bits in size, but only the first byte matters.
#       Also, you might want to change these to unsigned int for better cpu efficiency.
#       I made them unsigned char to illustrate that they were 8-bit results.
#
#       So to continue my C program example, you might do something like this
#
#       printf("The results of that conversion are Y: %u, U: %u, V: %u\n",
#               asm_rgb2yuv_result_y, asm_rgb2yuv_result_u, asm_rgb2yuv_result_v);
#
#       I hope this all makes sense. Good luck!
#       --Matt
#
#       PS : This is my first attempt at MMX programming so if it is lousy, sorry :)

.data

the_offset:
		.long 128
		.long 128

ystuff_q:
		.word 9798
		.word 19235
		.word 3736
		.word 0

ustuff_q:
		.word -5537
		.word -10878
		.word 16384
		.word 0
		
vstuff_q:
		.word 16384
		.word -13729
		.word -2664
		.word 0

.globl _asm_rgb2yuv_input
_asm_rgb2yuv_input:
		.space 8

.globl _asm_rgb2yuv_result_y
_asm_rgb2yuv_result_y:
		.space 8

.globl _asm_rgb2yuv_result_u
_asm_rgb2yuv_result_u:
		.space 8

.globl _asm_rgb2yuv_result_v
_asm_rgb2yuv_result_v:
		.space 8


.text

.globl	_asm_rgb2yuv
_asm_rgb2yuv:

        pushl %ebp

        movq _asm_rgb2yuv_input,%mm1
        movq the_offset,%mm7

        # Calculate Y
        movq ystuff_q,%mm0
        pmaddwd %mm1,%mm0
        movq %mm0,%mm2
        psrlq $32,%mm2
        paddd %mm2,%mm0
        psrad $15,%mm0  # divide by 32768
        movq %mm0, _asm_rgb2yuv_result_y

        # Calculate U
        movq ustuff_q,%mm0
        pmaddwd %mm1,%mm0
        movq %mm0,%mm2
        psrlq $32,%mm2
        paddd %mm2,%mm0
        psrad $15,%mm0  # divide by 32768
        paddd %mm7,%mm0 # add 128
        movq %mm0, _asm_rgb2yuv_result_u

        # Calculate V
        movq vstuff_q,%mm0
        pmaddwd %mm1,%mm0
        movq %mm0,%mm2
        psrlq $32,%mm2
        paddd %mm2,%mm0
        psrad $15,%mm0  # divide by 32768
        paddd %mm7,%mm0 # add 128
        movq %mm0, _asm_rgb2yuv_result_v

        #####################

        emms

        popl %ebp

        ret
