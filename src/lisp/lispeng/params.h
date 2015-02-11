#ifdef X_DEBUG //!!!D
#	define UCFG_LISP_FFI 0
#	define UCFG_LISP_DEBUG 1
#	define UCFG_LISP_FAST_EVAL_ATOMS 1
#endif 

const size_t MULTIPLE_VALUES_LIMIT = 50;  // exclusive limit, can be MULTIPLE_VALUES_LIMIT-1 values
const size_t JMPBUFSIZE = sizeof(jmp_buf); //!!!
//!!!const MAX_TOKEN_LEN = 256;

const size_t LISP_SP_SIZE = 10000;

#ifdef _DEBUG
  const size_t LISP_STACK_SIZE = 40000;
#else
  const size_t LISP_STACK_SIZE = 100000;
#endif

const size_t STACK_WARNING_SIZE = 100;

const size_t SUBR_R_FUNCS = 50, //Try 64, !
						 SUBR_FUNCS = 512-SUBR_R_FUNCS,
			       SUBR_OPS = 64;


#if UCFG_64
	const int LISP_BITS_IN_CP = 64;
	const int CP_VALUE_BITS = 56;
#else
	const int LISP_BITS_IN_CP = 32;
	const int CP_VALUE_BITS = 24;
#endif

const int VAL_BITS = LISP_BITS_IN_CP-8;
const int INT_BITS = VAL_BITS;

//!!!const INIT_HEAPSIZE_RECORD    = 131072;

const size_t HEAP_APP = 100,
      DEFAULT_INIT_HEAPSIZE = 256,
#if UCFG_WCE
	INIT_HEAPSIZE_CONS      = 400000,
    INIT_HEAPSIZE_SYMBOL    = 20000,
	INIT_HEAPSIZE_ARRAY     = 20000,
    INIT_HEAPSIZE_INTFUNC   = 20000,
#else
	INIT_HEAPSIZE_CONS      = 800000,
    INIT_HEAPSIZE_SYMBOL    = 40000,
	INIT_HEAPSIZE_ARRAY     = 100000, //!!! 4096
    INIT_HEAPSIZE_INTFUNC   = 40000,
#endif

//!!!      INIT_HEAPSIZE_OBJECT    = 1024,

//!!!R      INIT_HEAPSIZE_FOREIGNLIB   = 8,
      INIT_HEAPSIZE_CLOSURE   = 1024,
      INIT_HEAPSIZE_FLOAT     = 256,
      INIT_HEAPSIZE_BIGNUM    = 32000,
      INIT_HEAPSIZE_PATHNAME  = 200,
      INIT_HEAPSIZE_STREAM    = 256,
      INIT_HEAPSIZE_PACKAGE   = 16,
      INIT_HEAPSIZE_READTABLE = 4,
      INIT_HEAPSIZE_WEAKPOINTER = 64,
      INIT_HEAPSIZE_HASHTABLE = 200;

const size_t LISP_ALIGN = 64;

#ifndef UCFG_LISP_TRACER
#	define UCFG_LISP_TRACER 0
#endif

#if UCFG_LISP_TRACER
#	define LISP_TRACER CTracer tracer(_self, fun, nArg);
#	define LISP_TRACER_NORMAL_EXIT tracer.m_bNormalExit = true;
#else
#	define LISP_TRACER
#	define LISP_TRACER_NORMAL_EXIT
#endif


#ifndef UCFG_LISP_MACRO_AS_SPECIAL_OPERATOR
#	define UCFG_LISP_MACRO_AS_SPECIAL_OPERATOR 1
#endif

#ifndef UCFG_LISP_CHECK_STACK
#	define UCFG_LISP_CHECK_STACK 1
#endif



#if defined(_DEBUG) //!!! || defined(_PROFILE)
	//#define C_LISP_QUICK_MACRO                  // disable by default
#endif


#if /*!!!defined(_DEBUG) ||*/ defined(_PROFILE) //!!!D
#	define UCFG_LISP_PROFILE 1
#endif

#ifndef UCFG_LISP_PROFILE
#	define UCFG_LISP_PROFILE 0
#endif



#if UCFG_STDSTL && UCFG_WCE
#	define UCFG_LISP_SJLJ 0					// longjmp_unwind not supported
#endif

/*!!!!
#ifndef UCFG_LISP_SJLJ
#	if !defined(_DEBUG) //&& !defined(_PROFILE)
#		define UCFG_LISP_SJLJ 1		//!!!	Release
#	else
#		define UCFG_LISP_SJLJ 1		//!!!
#	endif
#endif
*/

#ifndef UCFG_LISP_SJLJ
#	define UCFG_LISP_SJLJ (!UCFG_USE_POSIX && !(UCFG_STDSTL))   //!!!   UCFG_WCE no supported SJLJ in early versions
#endif



//!!!#define C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR    // disable by default

#ifndef UCFG_LISP_SPLIT_SYM
#	define UCFG_LISP_SPLIT_SYM 0
#endif

#define LISP_ALLOCA_JMPBUF 1

#define LISP_DYN_IDX_SAVE 1

#ifndef UCFG_LISP_TAIL_REC
#	if UCFG_WCE
#		define UCFG_LISP_TAIL_REC 0
#	else
#		define UCFG_LISP_TAIL_REC 2
#	endif
#endif

#ifndef UCFG_LISP_SPARE_STACK
#	define UCFG_LISP_SPARE_STACK 1
#endif

#ifndef UCFG_LISP_FAST_HOOKS
#	define UCFG_LISP_FAST_HOOKS 1
#endif

#ifndef UCFG_LISP_FAST_EVAL_ATOMS
#	if UCFG_GNUC_VERSION && UCFG_GNUC_VERSION <= 406
#		define UCFG_LISP_FAST_EVAL_ATOMS 1			//!!! bug in the gcc
#	else
#		define UCFG_LISP_FAST_EVAL_ATOMS 2
#	endif
#endif

#ifndef UCFG_LISP_DEBUG_FRAMES
#	define UCFG_LISP_DEBUG_FRAMES 0
#endif

#ifndef UCFG_LISP_DEBUGGER
#	define UCFG_LISP_DEBUGGER 1
#endif

#ifndef UCFG_LISP_DEBUG
#	define UCFG_LISP_DEBUG (UCFG_DEBUG || UCFG_PROFILE)
#endif

#if defined(_DEBUG) || defined(_PROFILE) //!!!
#	define UCFG_LISP_LAZY_EXPAND	0		// disable by default
#	define UCFG_LISP_TEST	1
#else
#	define UCFG_LISP_LAZY_EXPAND	0
#	define UCFG_LISP_TEST	0
#endif


#ifndef UCFG_USE_READLINE
#	ifdef HAVE_READLINE
#		define UCFG_USE_READLINE 1
#	else
#		define UCFG_USE_READLINE (!UCFG_WCE && !UCFG_WDM && UCFG_EXTENDED && !UCFG_PLATFORM_X64)
#	endif
#endif

#ifndef UCFG_LISP_LIGHT_VM_CONTEXT
#	define UCFG_LISP_LIGHT_VM_CONTEXT 1
#endif

#ifdef _DEBUG
#	define UCFG_LISP_MT 0		//!!!D
#endif

#ifndef UCFG_LISP_MT
#	define UCFG_LISP_MT (!UCFG_WCE)
#endif

#ifndef UCFG_LISP_FFI
#	define UCFG_LISP_FFI (UCFG_WIN32_FULL || UCFG_WDM)
#endif

#ifndef UCFG_LISP_BUILTIN_RANDOM_STATE
#	define UCFG_LISP_BUILTIN_RANDOM_STATE 0
#endif

#ifndef UCFG_LISP_GC_USE_ONLY_BITMAP
#	define UCFG_LISP_GC_USE_ONLY_BITMAP 1
#endif

#ifndef UCFG_LISP_INTERNAL_TIME_UNITS_PER_SECOND
#	define UCFG_LISP_INTERNAL_TIME_UNITS_PER_SECOND std::chrono::milliseconds
#endif

#ifndef UCFG_LISP_FRAME_AS_OFFSET
#	define UCFG_LISP_FRAME_AS_OFFSET 0
#endif
