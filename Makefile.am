build=@build@

AM_CPPFLAGS=-I src -pthread 
#AM_CXXFLAGS=-include src/el/ext.h


include am.makeinc


bin_PROGRAMS = lisp
bindatadir = $(bindir)

dist_lisp_SOURCES = ext_messages.h
nodist_lisp_SOURCES = lispmsg.h

if BUILD_LIBEXT
lisp_DEPENDENCIES = lisp.cat

bindata_DATA= lisp.cat init.bls


	lisp_SOURCES += $(libext_FILES)

src/el/ext.h.gch: src/el/ext.h
	$(CXXCOMPILE) $(CXXFLAGS) -x c++-header $<

ext_messages.msg h/ext_messages.h : src/el/libext/ext_messages.mc
	-mkdir h
	perl mc2msg.pl -h h $<
	cp h/ext_messages.msg .

lispmsg.msg lispmsg.h : src/lisp/lispeng/lispmsg.mc
	perl mc2msg.pl -h . $<

lisp.cat : ext_messages.msg lispmsg.msg
	gencat --new -o $@ $^


else

bindata_DATA=  init.bls

endif


BUILT_SOURCES =$(nodist_lisp_SOURCES) #src/el/ext.h.gch


src/el/stl/codecvt : src/el/stl/codecvt
	echo

src/el/stl/mutex : src/el/stl/mutex
	echo "---"

src/el/stl/dynamic_bitset : src/el/stl/dynamic_bitset
	echo

src/el/stl/regex : src/el/stl/regex
	echo "---"

src/el/stl/thread : src/el/stl/thread
	echo "---"


L = LISPINC=src/lisp/code/:clisp/ ./lisp --norc
L0 = $(L) -M comp1.bls -c
L2 = $(L) -M comp2.bls -c

compiler.fas : clisp/compiler.lisp
	$(L) -b -c $< -o $@

comp1.bls : src/lisp/code/init.lisp compiler.fas
	$(L) -b --lp . -D LOAD_COMPILER_FAS -o $@ src/lisp/code/empty ./


init.bls : f1/ f2/ comp2.bls \
		f2/backquote.fas			f2/defs2.fas				f2/loop.fas			       \
		f2/clos.fas					f2/clos-slots1.fas			f2/clos-method1.fas		f2/clos-methcomb1.fas	\
		f2/clos-genfun1.fas			f2/clos-genfun2a.fas		f2/clos-methcomb2.fas	f2/clos-genfun2b.fas	\
		f2/clos-method2.fas			f2/clos-genfun3.fas			f2/clos-dependent.fas	f2/clos-genfun4.fas	\
		f2/clos-method3.fas			f2/clos-methcomb3.fas		f2/clos-slots2.fas		f2/clos-slotdef2.fas	\
		f2/clos-stablehash2.fas		f2/clos-specializer2.fas	f2/clos-class4.fas		f2/clos-class5.fas	\
		f2/clos-slotdef3.fas		f2/clos-specializer3.fas	f2/clos-class6.fas		f2/clos-method4.fas	\
		f2/clos-methcomb4.fas		f2/clos-genfun5.fas			f2/clos-print.fas		f2/clos-custom.fas	\
 		f2/documentation.fas		f2/gray.fas					f2/fill-out.fas			f2/disassem.fas		\
 		f2/condition.fas			f2/loadform.fas				f2/pprint.fas			f2/runprog.fas		\
		f2/describe.fas				f2/reploop.fas
	$(L) -b --lp . --lp f1/ --lp f2/ -i ./ src/lisp/code/empty


comp2.bls : comp1.bls \
	f1/macro.fas f1/explambda.fas f1/seq.fas f1/sort.fas f1/print-builtin.fas f1/init.fas \
		f1/cl.fas 				f1/pathname.fas f1/macros.fas 	f1/break.fas f1/string.fas \
	    f1/reader.fas 			f1/math.fas 	f1/uclos.fas 	f1/print.fas \
		f1/macros1.fas 			f1/macros2.fas 	f1/byte.fas		f1/clisp.fas	f1/clos-class3.fas		\
	    f1/defmacro.fas			f1/defs1.fas 						\
    	f1/lambdalist.fas		f1/places.fas		f1/defpackage.fas	\
	    f1/type.fas				f1/subtypep.fas		\
    	f1/clos-package.fas		f1/clos-macros.fas	f1/clos-class0.fas	f1/clos-metaobject1.fas	\
	    f1/clos-slotdef1.fas	f1/clos-stablehash1.fas	f1/clos-specializer1.fas	f1/clos-class1.fas	\
    	f1/clos-class2.fas		f1/defstruct.fas	f1/format.fas f1/functions.fas	\
	    f1/international.fas 	f1/cmacros.fas
	$(L) -b --lp . --lp f1/ -i ./ -o $@ src/lisp/code/empty


f1/ f2/ :
	mkdir -p $@

f1/%.lib f1/%.fas : src/lisp/code/%.lisp
	$(L0) $< -o $@

f1/%.lib f1/%.fas : clisp/%.lisp
	$(L0) $< -o $@

f2/%.lib f2/%.fas : clisp/%.lisp
	$(L2) $< -o $@



CLEANFILES = src/el/ext.h.gch lisp.cat *.bls *.fas *.lib f1/*.fas f1/*.lib f2/*.fas f2/*.lib ext_messages.msg lispmsg.msg $(BUILT_SOURCES)
