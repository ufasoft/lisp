/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
// third field:
// 0: SYS
// 1: CL
// 2: CLOS;
// 3: EXT
// 7: CFFI-SYS

LFUN("CAR",                         F_Car,                        	1, 0, 1) // 0x0
LFUN("CDR",                         F_Cdr,                        	1, 0, 1)    
LFUN("CONS",                        F_Cons,                       	2, 0, 1)
LFUN("EQ",                          F_Eq,                         	2, 0, 1)
LFUN("EQL",                         F_Eql,                        	2, 0, 1) 
LFUN("EQUAL",                       F_Equal,                      	2, 0, 1)    
LFUN("EQUALP",						F_EqualP,					  	2, 0, 1)

LFUN("NULL",                        F_Null,                       	1, 0, 1)		
//!!!R LFUN_REPEAT("NOT",                  F_Null,                  		1, 0, 1)

LFUN("ZEROP",                       F_ZeroP,                      	1, 0, 1)		
LFUN("SVREF",                       F_SVRef,                      	2, 0, 1)
//!!!		LFUN("_ADD",                        F_Add,                      2, 0, 0),
//!!!    LFUN("_SUB",                        F_Sub,                        2, 0, 0),
LFUN("1+",							F_Inc,                        	1, 0, 1)
LFUN("1-",							F_Dec,                        	1, 0, 1)

LFUN("%STRUCTURE-REF",              F_StructureRef,               	3, 0, 0)
LFUN("%STRUCTURE-STORE",            F_StructureStore,             	4, 0, 0)

//!!!R LFUN("_MUL",                        F_Mul,                        	2, 0, 0)
LFUN("ROW-MAJOR-AREF",              F_RowMajorAref,               	2, 0, 1)
LFUN("SIMPLE-VECTOR-P",             F_SimpleVectorP,              	1, 0, 1)
LFUN("SIMPLE-ARRAY-P",              F_SimpleArrayP,               	1, 0, 0)
LFUN("_SETF_ROW-MAJOR-AREF",        F_SetfRowMajorAref,           	3, 0, 0)
LFUN("ARRAY-ELEMENT-TYPE",          F_ArrayElementType,           	1, 0, 1)

// Array functions
LFUN_K("MAKE-ARRAY",				F_MakeArray,                  	1, 0, 1, K_ELEMENT_TYPE K_INITIAL_ELEMENT K_INITIAL_CONTENTS K_ADJUSTABLE K_FILL_POINTER K_DISPLACED_TO K_DISPLACED_INDEX_OFFSET)
LFUN_K("ADJUST-ARRAY",				F_AdjustArray,                	2, 0, 1, K_ELEMENT_TYPE K_INITIAL_ELEMENT K_INITIAL_CONTENTS K_FILL_POINTER K_DISPLACED_TO K_DISPLACED_INDEX_OFFSET)
LFUN("VECTOR-PUSH-EXTEND",          F_VectorPushExtend,           	2, 1, 1)
LFUN("ARRAY-RANK",                  F_ArrayRank,                  	1, 0, 1)    
LFUN("ADJUSTABLE-ARRAY-P",          F_AdjustableArrayP,           	1, 0, 1)
//!!!R LFUN("_COPY-ARRAY",                 F_CopyArray,                  3, 0, 0)

LFUN("ARRAY-DIMENSIONS",            F_ArrayDimensions,            	1, 0, 1)  // 0x10
LFUN("ARRAY-DISPLACEMENT",          F_ArrayDisplacement,          	1, 0, 1)
LFUN("BIT-AND",                     F_BitAND,                     	2, 1, 1)
LFUN("BIT-IOR",                     F_BitIOR,                     	2, 1, 1)
LFUN("BIT-XOR",                     F_BitXOR,                     	2, 1, 1)
LFUN("BIT-EQV",                     F_BitEQV,                     	2, 1, 1)
LFUN("BIT-NAND",                    F_BitNAND,                    	2, 1, 1)
LFUN("BIT-NOR",                     F_BitNOR,                     	2, 1, 1)
LFUN("BIT-ANDC1",                   F_BitANDC1,                   	2, 1, 1)
LFUN("BIT-ANDC2",                   F_BitANDC2,                   	2, 1, 1)
LFUN("BIT-ORC1",                    F_BitORC1,                    	2, 1, 1)
LFUN("BIT-ORC2",                    F_BitORC2,                    	2, 1, 1)
LFUN("BIT-NOT",                     F_BitNOT,                     	1, 1, 1)
LFUN("ARRAY-HAS-FILL-POINTER-P",    F_ArrayHasFillPointerP,       	1, 0, 1)
LFUN("FILL-POINTER",                F_FillPointer,                	1, 0, 1)
LFUN("SET-FILL-POINTER",			F_SetFillPointer,				2, 0, 0)
LFUN("GETF",                        F_Getf,                       	2, 1, 1)
LFUN("%REMF",                       F_PRemf,                      	2, 0, 0)
LFUN("%PUTF",                       F_PPutf,                      	3, 0, 0)
LFUN("GET",							F_Get,							2, 1, 1)
LFUN("%PUT",						F_PPut,							3, 0, 0)
LFUN("_SETF_MACRO-FUNCTION",        F_SetfMacroFunction,          	2, 0, 0)
LFUN("MACRO-EXPANDER",              F_MacroExpander,              	1, 0, 0)
LFUN("MACRO-LAMBDA-LIST",           F_MacroLambdaList,            	1, 0, 0)
LFUN("STANDARD-CHAR-P",             F_StandardCharP,              	1, 0, 1)
LFUN("%RPLACA",						F_PRplacA,                    	2, 0, 0)
LFUN("%RPLACD",						F_PRplacD,                    	2, 0, 0)
LFUN("RPLACA",						F_RplacA,						2, 0, 1)
LFUN("RPLACD",						F_RplacD,						2, 0, 1)

// Char functions
LFUN("ALPHA-CHAR-P",                F_AlphaCharP,                 	1, 0, 1)
LFUN("ALPHANUMERICP",               F_AlphanumericP,              	1, 0, 1)    
LFUN("DIGIT-CHAR-P",                F_DigitCharP,                 	1, 1, 1)    
LFUN("DIGIT-CHAR",                  F_DigitChar,                  	1, 1, 1)    

LFUN("UPPER-CASE-P",                F_UpperCaseP,                 	1, 0, 1)
LFUN("LOWER-CASE-P",                F_LowerCaseP,                 	1, 0, 1)
LFUN("BOTH-CASE-P",                 F_BothCaseP,                  	1, 0, 1)

LFUN("BOUNDP",                      F_Boundp,                     	1, 0, 1)
LFUN("MAKUNBOUND",                  F_Makunbound,                 	1, 0, 1)
LFUN("FBOUNDP",						F_FBoundp,                    	1, 0, 1)
LFUN("FDEFINITION",					F_FDefinition,                	1, 0, 1)
LFUN("SYMBOL-FUNCTION",				F_SymbolFunction,             	1, 0, 1)
LFUN("FMAKUNBOUND",                 F_FMakunbound,                	1, 0, 1)
LFUN("_SETF_FUNC-NAME",             F_SetfFuncName,               	2, 0, 0)
LFUN("SYMBOL-NAME",                 F_SymbolName,                 	1, 0, 1)                                                                  
LFUN("SYMBOL-VALUE",                F_SymbolValue,                	1, 0, 1)
LFUN("SET",                         F_Set,                        	2, 0, 1)
LFUN("MAKE-SYMBOL",                 F_MakeSymbol,                 	1, 0, 1)

LFUN("SYMBOL-PACKAGE",              F_SymbolPackage,              	1, 0, 1)
LFUN("CONSTANTP",                   F_ConstantP,                  	1, 1, 1)
                                                              
//Package Functions
LFUN("FIND-PACKAGE",				F_FindPackage,                	1, 0, 1)
LFUN("PACKAGE-NAME",                F_PackageName,                	1, 0, 1)
LFUN("PACKAGE-NICKNAMES",           F_PackageNicknames,           	1, 0, 1)
LFUN("LIST-ALL-PACKAGES",           F_ListAllPackages,            	0, 0, 1)
LFUN("PACKAGE-USE-LIST",            F_PackageUseList,             	1, 0, 1)
LFUN("PACKAGE-USED-BY-LIST",        F_PackageUsedByList,         	1, 0, 1)
LFUN("USE-PACKAGE",            		F_UsePackage,             		1, 1, 1)
LFUN("UNUSE-PACKAGE",            	F_UnusePackage,             	1, 1, 1)
LFUN("DELETE-PACKAGE",             	F_DeletePackage,              	1, 0, 1)
LFUN("PACKAGE-SHADOWING-SYMBOLS",   F_PackageShadowingSymbols,    	1, 0, 1)
LFUN("SHADOW",                   	F_Shadow,                   	1, 1, 1)
LFUN("SHADOWING-IMPORT",          	F_ShadowingImport,           	1, 1, 1)
LFUN("IMPORT",                     	F_Import,                     	1, 1, 1)
LFUN("EXPORT",                     	F_Export,                     	1, 1, 1)
LFUN("UNEXPORT",                  	F_Unexport,                   	1, 1, 1)
//!!!R LFUN("%EXPORT",                     F_SysExport,                  	1, 0, 0)

LFUN_K("MAKE-PACKAGE",				F_MakePackage,                	1, 0, 1, K_NICKNAMES K_USE)
LFUN("RENAME-PACKAGE",              F_RenamePackage,              	2, 1, 1)
LFUN("INTERN",                      F_Intern,                     	1, 1, 1)
LFUN("UNINTERN",                   	F_Unintern,                   	1, 1, 1)

LFUN("GETHASH",                     F_GetHash,                    	2, 1, 1)    
LFUN("PUTHASH",                     F_PutHash,                    	3, 0, 0)
LFUN("REMHASH",                     F_RemHash,                    	2, 0, 1)
LFUN("HASH-TABLE-SIZE",             F_HashTableSize,              	1, 0, 1)
LFUN("CLRHASH",                     F_ClrHash,                    	1, 0, 1)
LFUN("HASH-TABLE-COUNT",            F_HashTableCount,             	1, 0, 1)
LFUN("HASH-TABLE-ITERATOR",        	F_HashTableIterator,          	1, 0, 0)
LFUN("HASH-TABLE-ITERATE",         	F_HashTableIterate,           	1, 0, 0)
LFUN("SXHASH",                      F_SxHash,                     	1, 0, 1)
LFUN("COPY-READTABLE",              F_CopyReadtable,              	0, 2, 1)
LFUN("_RMHASH",                     F_M_Sharp,                    	2, 0, 0)
LFUN("_RMQUOTE",                    F_M_Quote,                    	2, 0, 0)
                                                              
LFUN("_RMDQUOTE",                   F_M_DQuote,                   	2, 0, 0) // 0x50
LFUN("_RMBQUOTE",                   F_M_BQuote,                   	2, 0, 0)
LFUN("_RMCOMMA",                    F_M_Comma,                    	2, 0, 0)
LFUN("_RMLPAR",                     F_M_LeftParenthesis,          	2, 0, 0)
LFUN("_RMRPAR",                     F_M_RightParenthesis,         	2, 0, 0)
LFUN("_RMSEMI",                     F_M_Semicolon,                	2, 0, 0)
//!!!R LFUN("_READ-CLOSURE",               F_ReadClosure,                	3, 0, 0) //!!!D
LFUN("READ-CHAR",                   F_ReadChar,                   	0, 4, 1)
LFUN("READ-CHAR-NO-HANG",           F_ReadCharNoHang,             	0, 4, 1)
LFUN("LISTEN",						F_Listen,						0, 1, 1)
LFUN("CLEAR-INPUT",					F_ClearInput,					0, 1, 1)
LFUN("WRITE-CHAR",                  F_WriteChar,                  	1, 1, 1)
LFUN("UNREAD-CHAR",                 F_UnreadChar,                 	1, 1, 1)
LFUN("PEEK-CHAR",                   F_PeekChar,                   	0, 5, 1)
LFUN("MAKE-READ-LABEL",             F_MakeReadLabel,              	1, 0, 0)
LFUN("_DISASSEMBLE",                F_Disassemble,                	1, 0, 0)
LFUN("_FIND-IN-LISPENV",            F_FindInLispenv,              	1, 0, 0)
//!!!    LFUN("_LPAR-READER",                F_LParReader,                2, 0, 0),
LFUN("%%TIME",                      F_PPTime,                       0, 0, 0)
LFUN("ROOM",                      	F_Room,                       	0, 1, 1)
LFUN("_INVOKE-HANDLERS",            F_InvokeHandlers,             	1, 0, 0)

LFUN("READTABLE-CASE",              F_ReadtableCase,              	1, 0, 1)
LFUN("_SETF_READTABLE-CASE",        F_SetfReadtableCase,          	2, 0, 0)

LFUN("HASH-TABLE-REHASH-SIZE",      F_HashTableRehashSize,			1, 0, 1) // 0x60
LFUN("HASH-TABLE-TEST",             F_HashTableTest,				1, 0, 1)
LFUN("HASH-TABLE-REHASH-THRESHOLD", F_HashTableRehashThreshold,		1, 0, 1)
LFUN("FINISH-OUTPUT",               F_FinishOutput,					0, 1, 1)
LFUN("FORCE-OUTPUT",                F_ForceOutput,					0, 1, 1)
LFUN("CLEAR-OUTPUT",                F_ClearOutput,					0, 1, 1)                                                                  
LFUN("_PRINT-FLOAT",                F_PrintFloat,					2, 0, 0)
LFUN("_PRINT-SYMBOL-NAME",          F_PrintSymbolName,				2, 0, 0)
LFUN("%PPRINT-LOGICAL-BLOCK",       F_PPrintLogicalBlock,			3, 0, 0)
LFUN("PPRINT-INDENT",				F_PPrintIndent,					2, 1, 1)
LFUN("PPRINT-NEWLINE",				F_PPrintNewline,				1, 1, 1)
LFUN("FORMAT-TABULATE",				F_FormatTabulate,				3, 2, 0)
//!!!R LFUN("_PRINT-INT",									F_PrintInt,				            2, 0, 0)
//!!!R LFUN("_DIV",                        F_Div,                        	2, 0, 0)

//!!!R LFUN("_REV-LIST-TO-STRING",         F_RevListToString,            1, 0, 0)

LFUN("TRUNCATE",                    F_Truncate,                   	1, 1, 1)
//!!!R LFUN("_MAKE-RATIO",                 F_MakeRatio,                  	2, 0, 0)

LFUN("CHAR-CODE",                   F_CharCode,                   	1, 0, 1)
//!!!R LFUN_REPEAT("CHAR-INT",             F_CharCode,                   	1, 0, 1)

LFUN("CODE-CHAR",                   F_CodeChar,                   	1, 0, 1)
LFUN("CHAR-UPCASE",                 F_CharUpcase,                 	1, 0, 1)
LFUN("CHAR-DOWNCASE",               F_CharDowncase,               	1, 0, 1)

LFUN_K("STRING-UPCASE",             F_StringUpcase,              	1, 0, 1, K_START K_END)
LFUN_K("STRING-DOWNCASE",           F_StringDowncase,               1, 0, 1, K_START K_END)

LFUN("EVENP",                       F_EvenP,                      	1, 0, 1)
LFUN("ABS",                       	F_Abs,                      	1, 0, 1)
                                                              
LFUN("MAKE-MACRO",                  F_MakeMacro,                  	1, 1, 0)
LFUN("PACKAGE-DOCUMENTATION",       F_PackageDocumentation,       	1, 0, 0)
LFUN("(SETF PACKAGE-DOCUMENTATION)", F_SetfPackageDocumentation,   	2, 0, 0)
LFUN("IDENTITY",                    F_Identity,                   	1, 0, 1)

LFUN("GET-INTERNAL-REAL-TIME",      F_GetInternalRealTime,        	0, 0, 1)
LFUN("GET-INTERNAL-RUN-TIME",       F_GetInternalRunTime,         	0, 0, 1)
LFUN("GET-UNIVERSAL-TIME",          F_GetUniversalTime,           	0, 0, 1)
LFUN("DEFAULT-TIME-ZONE",           F_DefaultTimeZone,            	0, 0, 0)
LFUN("_DECODE-UNIVERSAL-TIME",      F_DecodeUniversalTime,        	2, 0, 0)
LFUN("_ENCODE-UNIVERSAL-TIME",      F_EncodeUniversalTime,        	7, 0, 0)

// Math functions
LFUN("_LOG",                        F_Log,                        	1, 0, 0)
LFUN("_SQRT",                       F_Sqrt,                       	1, 0, 0)
LFUN("_EXP",                        F_Exp,                        	1, 0, 0)
LFUN("_SIN",                        F_Sin,                        	1, 0, 0)
LFUN("_COS",                        F_Cos,                        	1, 0, 0)
LFUN("_ASIN",                       F_ASin,                       	1, 0, 0)
LFUN("_ACOS",                       F_ACos,                       	1, 0, 0)
LFUN("_ATAN",                       F_ATan,                       	1, 0, 0)
LFUN("_ATAN2",                      F_ATan2,                      	2, 0, 0)
LFUN("PHASE",                       F_Phase,                      	1, 0, 1)
LFUN("SIGNUM",                      F_Signum,                     	1, 0, 1)

LFUN("DECODE-FLOAT",                F_DecodeFloat,                	1, 0, 1)
LFUN("FLOAT-DIGITS",               	F_FloatDigits,               	1, 0, 1)
LFUN("SCALE-FLOAT",                 F_ScaleFloat,                 	2, 0, 1)
LFUN("FLOAT-SIGN",                  F_FloatSign,                  	1, 1, 1)
LFUN("INTEGER-DECODE-FLOAT",        F_IntegerDecodeFloat,         	1, 0, 1)

LFUN("MINUSP",                      F_MinusP,                     	1, 0, 1)
LFUN("PLUSP",                       F_PlusP,                      	1, 0, 1)
LFUN("FIND-SYMBOL",                 F_FindSymbol,                 	1, 1, 1)

//!!!D    LFUN("FIND-ALL-SYMBOLS",            F_FindAllSymbols,            1, 0, 1),
LFUN("MAP-SYMBOLS",					F_MapSymbols,					2, 0, 0)
LFUN("MAP-ALL-SYMBOLS",				F_MapAllSymbols,              	1, 0, 0)
//!!!D    LFUN("_GET-SYMBOLS",                F_GetSymbols,                1, 0, 0),
//!!!R LFUN("_GET-EXTERNAL-SYMBOLS",       F_GetExternalSymbols,         1, 0, 0)
LFUN("_Sharp_VerticalBar",          F_Sharp_VerticalBar,          	3, 0, 0)
LFUN("_Sharp_Quote",                F_Sharp_Quote,                	3, 0, 0)
LFUN("_Sharp_Backslash",            F_Sharp_Backslash,            	3, 0, 0)
LFUN("_Sharp_Dot",                  F_Sharp_Dot,                  	3, 0, 0)
LFUN("_Sharp_Colon",                F_Sharp_Colon,                	3, 0, 0)
LFUN("_Sharp_Plus",                 F_Sharp_Plus,                 	3, 0, 0)
LFUN("_Sharp_Minus",                F_Sharp_Minus,                	3, 0, 0)
LFUN("_Sharp_LPar",                 F_Sharp_LPar,                 	3, 0, 0)
LFUN("_Sharp_Asterisk",             F_Sharp_Asterisk,             	3, 0, 0)

LFUN("_PATHNAME-FIELD-EX",          F_PathnameFieldEx,            	2, 0, 0) // 0xA0
//!!!R LFUN("_Sharp_H",                    F_Sharp_H,                    3, 0, 0),
LFUN("_Sharp_Y",                    F_Sharp_Y,                    	3, 0, 0)
LFUN("_Sharp_S",                    F_Sharp_S,                    	3, 0, 0)

// File function
LFUN("FILE-WRITE-DATE",             F_FileWriteDate,              	1, 0, 1)
LFUN("PROBE-FILE",                  F_ProbeFile,                  	1, 0, 1)
LFUN("FILE-AUTHOR",					F_FileAuthor,   				1, 0, 1)
LFUN("DELETE-FILE",                 F_DeleteFile,                 	1, 0, 1)

LFUN("RENAME-FILE",					F_RenameFile,                 	2, 0, 1)
LFUN_K("DIRECTORY",                 F_Directory,                  	1, 0, 1, K_SPECIAL_ALLOW_OTHER_KEYS)
LFUN_K("ENSURE-DIRECTORIES-EXIST",  F_EnsureDirectoriesExist,     	1, 0, 1, K_VERBOSE)
LFUN("DELETE-DIRECTORY",  			F_DeleteDirectory,     			1, 0, 3)
LFUN("GET-MACRO-CHARACTER",         F_GetMacroCharacter,          	1, 1, 1)
LFUN("SET-MACRO-CHARACTER",         F_SetMacroCharacter,          	2, 2, 1)
LFUN("GET-DISPATCH-MACRO-CHARACTER", F_GetDispatchMacroCharacter,  	2, 1, 1)
LFUN("SET-DISPATCH-MACRO-CHARACTER", F_SetDispatchMacroCharacter,  	3, 1, 1)
LFUN("READ-DELIMITED-LIST",         F_ReadDelimitedList,          	1, 3, 1) //!!! non-standard &optional arg

// Predicates
LFUN("SYMBOLP",                     F_SymbolP,                    	1, 0, 1)
LFUN("CHARACTERP",                  F_CharacterP,                 	1, 0, 1)
LFUN("NUMBERP",                     F_NumberP,                    	1, 0, 1)
LFUN("REALP",                       F_RealP,                      	1, 0, 1)
LFUN("INTEGERP",                    F_IntegerP,                   	1, 0, 1)
LFUN("RATIONALP",                   F_RationalP,                  	1, 0, 1)
LFUN("FLOATP",                      F_FloatP,                     	1, 0, 1)                                                                  
LFUN("ARRAYP",                      F_ArrayP,                     	1, 0, 1)
LFUN("VECTORP",                     F_VectorP,                    	1, 0, 1)
LFUN("STRINGP",                     F_StringP,                    	1, 0, 1)
LFUN("PATHNAMEP",                   F_PathnameP,                    1, 0, 1)
LFUN("STREAMP",                     F_StreamP,                    	1, 0, 1)
LFUN("TYPE-OF",                     F_TypeOf,                     	1, 0, 1)
LFUN("CLASS-OF",                    F_ClassOf,                    	1, 0, 1)

// List functions
LFUN("ATOM",						F_Atom,                       	1, 0, 1)
LFUN("CONSP",                       F_ConsP,                      	1, 0, 1)
LFUN("MEMQ",                        F_Memq,                       	2, 0, 0)
LFUN("ENDP",                        F_Endp,                       	1, 0, 1)
LFUN("LENGTH",                      F_Length,                     	1, 0, 1)
LFUN("%PROPER-LIST",				F_PProperList,					1, 0, 0)
LFUN("VALUES-LIST",					F_ValuesList,					1, 0, 1)
LFUN("LIST-LENGTH",					F_ListLength,					1, 0, 1)
LFUN("LIST-LENGTH-DOTTED",			F_ListLengthDotted,				1, 0, 3)
LFUN("LAST",                        F_Last,                       	1, 1, 1)
LFUN("COPY-LIST",                   F_CopyList,                   	1, 0, 1)
LFUN("NREVERSE",                    F_NReverse,                   	1, 0, 1)
LFUN("NRECONC",						F_NReconc,						2, 0, 1)
LFUN_K("MEMBER",					F_Member,						2, 0, 1, K_KEY K_TEST K_TEST_NOT )
LFUN_K("ASSOC",						F_Assoc,						2, 0, 1, K_KEY K_TEST K_TEST_NOT )
LFUN_K("NSUBLIS",					F_NSublis,						2, 0, 1, K_KEY K_TEST K_TEST_NOT )
LFUN_K("MAKE-LIST",					F_MakeList,						1, 0, 1, K_INITIAL_ELEMENT )
LFUN("LISTP",                       F_ListP,                      	1, 0, 1)
LFUN("NTHCDR",                      F_NthCdr,                     	2, 0, 1)
LFUN("ELT",                         F_Elt,							2, 0, 1)
LFUN("(SETF ELT)",                  F_SetfElt,						3, 0, 0)
LFUN("_FIND-SEQ-TYPE",              F_FindSeqType,                	1, 0, 0)
LFUN("_GET-SEQ-TYPE",               F_GetSeqType,                 	1, 0, 0)
LFUN("_GET-VALID-SEQ-TYPE",         F_GetValidSeqType,            	1, 0, 0)
LFUN("_GET-TD-CHECK-INDEX",         F_GetTdCheckIndex,            	2, 0, 0)
LFUN("_SEQ-ITERATE",                F_SeqIterate,                 	6, 0, 0)
LFUN("_SEQ-TEST",                   F_SeqTest,                     	7, 0, 0)
LFUN("SUBSEQ",                      F_Subseq,                     	2, 1, 1)


//!!!D    LFUN("_READ-SIMPLE-TOKEN",          F_ReadSimpleToken,           2, 0, 0),
LFUN("ED",                          F_ED,                         	0, 1, 1)

LFUN("MAKE-ECHO-STREAM",            F_MakeEchoStream,             	2, 0, 1)
LFUN("MAKE-SYNONYM-STREAM",         F_MakeSynonymStream,          	1, 0, 1)
LFUN("MAKE-TWO-WAY-STREAM",         F_MakeTwoWayStream,           	2, 0, 1)
LFUN("ECHO-STREAM-INPUT-STREAM",    F_EchoStreamInputStream,      	1, 0, 1)
LFUN("ECHO-STREAM-OUTPUT-STREAM",   F_EchoStreamOutputStream,     	1, 0, 1)
LFUN("SYNONYM-STREAM-SYMBOL",       F_SynonymStreamSymbol,        	1, 0, 1)
LFUN("TWO-WAY-STREAM-INPUT-STREAM", F_TwoWayStreamInputStream,    	1, 0, 1)
LFUN("TWO-WAY-STREAM-OUTPUT-STREAM", F_TwoWayStreamOutputStream,   	1, 0, 1)
LFUN("CONCATENATED-STREAM-STREAMS", F_ConcatenatedStreamStreams,  	1, 0, 1)
LFUN("BROADCAST-STREAM-STREAMS",    F_BroadcastStreamStreams,     	1, 0, 1)
LFUN("STREAM-EXTERNAL-FORMAT",      F_StreamExternalFormat,       	1, 0, 1)
LFUN("BUILT-IN-STREAM-P",			F_BuiltInStreamP,				1, 0, 0)
LFUN("BUILT-IN-STREAM-ELEMENT-TYPE", F_BuiltInStreamElementType,   	1, 0, 0)
LFUN("INTERACTIVE-STREAM-P",        F_InteractiveStreamP,         	1, 0, 1)
LFUN("BUILT-IN-STREAM-OPEN-P",      F_BuiltInStreamOpenP,         	1, 0, 0)
LFUN("LINE-POSITION",               F_LinePosition,               	0, 1, 0)
LFUN("LINE-NUMBER",                 F_LineNumber,                 	1, 0, 0)
LFUN("STREAM-FASL-P",               F_StreamFaslP,                	1, 1, 0)

LFUN("_MAKE-STREAM",                F_MakeStream,                 	1, 2, 0)
LFUN_K("OPEN",                      F_Open,                       	1, 0, 1, K_DIRECTION K_ELEMENT_TYPE K_IF_EXISTS K_IF_DOES_NOT_EXIST K_EXTERNAL_FORMAT)
LFUN_K("BUILT-IN-STREAM-CLOSE",     F_Close,                      	1, 0, 0, K_ABORT)
LFUN("FILE-POSITION",               F_FilePosition,               	1, 1, 1)

LFUN("MAKE-STRING-INPUT-STREAM",    F_MakeStringInputStream,      	1, 2, 1)
LFUN("MAKE-STRING-PUSH-STREAM",     F_MakeStringPushStream,       	1, 1, 0)
LFUN("STRING-INPUT-STREAM-INDEX",   F_StringInputStreamIndex,     	1, 0, 0)
LFUN("GET-OUTPUT-STREAM-STRING",    F_GetOutputStreamString,      	1, 0, 1)

LFUN("INPUT-STREAM-P",              F_InputStreamP,               	1, 0, 1) // 0xD0
LFUN("OUTPUT-STREAM-P",             F_OutputStreamP,               	1, 0, 1)
LFUN("READ-BYTE",                   F_ReadByte,                   	1, 2, 1)
LFUN("WRITE-BYTE",                  F_WriteByte,                  	2, 0, 1)
LFUN("FILE-LENGTH",                 F_FileLength,                 	1, 0, 1)                         
LFUN("%PUTD",                       F_Putd,                       	2, 0, 0)

LFUN("_GET-CLOSURE",                F_GetClosure,                 	3, 0, 0)
LFUN("PROCLAIM",           			F_Proclaim,            			1, 0, 1)
LFUN("%PROCLAIM-CONSTANT",          F_ProclaimConstant,           	2, 0, 0)
LFUN("%PROCLAIM-SYMBOL-MACRO",      F_ProclaimSymbolMacro,        	1, 0, 0)
LFUN("MACRO-FUNCTION",              F_MacroFunction,              	1, 1, 1)
LFUN("FUNCTION-MACRO-FUNCTION",     F_FunctionMacroFunction,      	1, 0, 0)
LFUN("FUNCTION-MACRO-EXPANDER",     F_FunctionMacroExpander,      	1, 0, 0)
LFUN("MAKE-FUNCTION-MACRO",			F_MakeFunctionMacro,			2, 0, 0)
LFUN("SYMBOL-PLIST",                F_SymbolPList,                	1, 0, 1)
LFUN("%PUTPLIST",					F_PPutPlist,					2, 0, 0)
LFUN("GENSYM",                      F_GenSym,                     	0, 1, 1)
LFUN("MAKE-CODE-VECTOR",			F_MakeCodeVector,				1, 0, 0)
LFUN("%MAKE-CLOSURE",				F_MakeClosure,					4, 2, 0)
//!!!R LFUN("%PUTD-CLOSURE",               F_PutdClosure,               3, 0, 0),
//!!!R LFUN("_ERR1",                       F_Err1,                       	3, 0, 0)
LFUN("_GET-ERR-MESSAGE",            F_GetErrMessage,              	1, 0, 0)
LFUN("_OBJ-PTR",                    F_ObjPtr,                     	1, 0, 0) //!!!R
//!!!LFUN("RATIONAL",                    F_Rational,                   1, 0, 1)
                                                              
LFUN("NUMERATOR",                   F_Numerator,                  	1, 0, 1) // 0xE0
LFUN("DENOMINATOR",                 F_Denominator,                	1, 0, 1)

#if UCFG_LISP_BUILTIN_RANDOM_STATE
LFUN("RANDOM-STATE-P",              F_RandomStateP,               	1, 0, 1)
#endif

LFUN("RANDOM",                      F_Random,                     	1, 1, 1)
LFUN("MAKE-RANDOM-STATE",           F_MakeRandomState,            	0, 1, 1)

LFUN("ASH",                         F_ASH,                        	2, 0, 1)
LFUN("COMPLEX",                     F_Complex,                    	1, 1, 1)
LFUN("REALPART",                    F_RealPart,                   	1, 0, 1)
LFUN("IMAGPART",                    F_ImagPart,                   	1, 0, 1)
LFUN("LOGNAND",                     F_LogNAND,                    	2, 0, 1)
LFUN("LOGNOR",                      F_LogNOR,                     	2, 0, 1)

LFUN("MACROEXPAND-1",               F_Macroexpand1,               	1, 1, 1)
LFUN("_UPDATE-TRACE",               F_UpdateTrace,                  0, 0, 0)
LFUN("_CHAR-TYPE",                  F_CharType,                   	1, 1, 0)                                                                  
//!!!D LFUN("_SETF_CHAR-TYPE",             F_SetfCharType,               4, 0, 0),
//!!!    LFUN("_STRING=",                    F_StringEqual,                2, 0, 0),
//!!!R LFUN("_CHAIN-S-EX",                 F_ChainsEx,                   	3, 0, 0)
LFUN("_LOAD",                       F_Load,                        	1, 0, 0) // replaced after BOOTSTRAP
//!!!D    LFUN("_FLOAT-TOKEN-P",              F_FloatTokenP,                1, 0, 0),
LFUN("FLOAT",                     	F_Float,                      	1, 1, 1)
//!!!D    LFUN("_FLOOR1",                     F_Floor1,                     1, 0, 0),
LFUN("EVAL",                        F_Eval,                       	1, 0, 1)
LFUN("EVALHOOK",                    F_Evalhook,                   	3, 1, 3)

LFUN("LOGANDC1",                    F_LogANDC1,                   	2, 0, 1) // 0x100
LFUN("LOGANDC2",                    F_LogANDC2,                   	2, 0, 1)
LFUN("LOGORC1",                     F_LogORC1,                    	2, 0, 1)
LFUN("LOGORC2",                     F_LogORC2,                    	2, 0, 1)
LFUN("LOGNOT",                      F_LogNOT,                     	1, 0, 1)
LFUN("LOGTEST",                     F_LogTest,                    	2, 0, 1)
LFUN("LOGCOUNT",                    F_LogCount,                  	1, 0, 1)
LFUN("READ",                        F_Read,                       	0, 4, 1)                                                                  
LFUN("READ-PRESERVING-WHITESPACE",  F_ReadPreservingWhitespace,   	0, 4, 1)                                                                  
LFUN("STRING",                      F_String,                     	1, 0, 1)
//!!!RLFUN("%PRINT",                      F_S_Print,                    	1, 0, 0)
LFUN("STRUCTURE-OBJECT-P",          F_StructureObjectP,           	1, 0, 2)
LFUN("COMPILED-FUNCTION-P",         F_CompiledFunctionP,          	1, 0, 1)
LFUN("GENERIC-FUNCTION-P",          F_GenericFunctionP,           	1, 0, 2)

LFUN("%FUNTABREF",                  F_FunTabRef,                  	1, 0, 0)
LFUN("SUBR-INFO",                   F_SubrInfo,                   	1, 0, 0)
LFUN("FUNCTION-NAME-P",             F_FunctionNameP,              	1, 0, 0)
LFUN("PARSE-BODY",                  F_ParseBody,                  	1, 1, 0)
LFUN("MACROP",                      F_MacroP,                     	1, 0, 0)
LFUN("SYMBOL-MACRO-P",              F_SymbolMacroP,               	1, 0, 0)
LFUN("MAKE-SYMBOL-MACRO",           F_MakeSymbolMacro,            	1, 0, 0)
LFUN("MAKE-GLOBAL-SYMBOL-MACRO",    F_MakeGlobalSymbolMacro,      	1, 0, 0)
LFUN("SYMBOL-MACRO-EXPAND",         F_SymbolMacroExpand,          	1, 0, 3)
LFUN("GLOBAL-SYMBOL-MACRO-DEFINITION", F_GlobalSymbolMacroDefinition, 1, 0, 0)
LFUN("SPECIAL-VARIABLE-P",          F_SpecialVariableP,           	1, 1, 3)

LFUN("CLOSUREP",                    F_ClosureP,                   	1, 0, 0)
LFUN("FUNCTIONP",                   F_FunctionP,                  	1, 0, 1)
LFUN("CLOSURE-NAME",				F_ClosureName,					1, 0, 0)
LFUN("(SETF CLOSURE-NAME)",         F_SetfClosureName,				2, 0, 0)
LFUN("CLOSURE-CONSTS",				F_ClosureConsts,              	1, 0, 0)
LFUN("CLOSURE-CONST",				F_ClosureConst,					2, 0, 0)
LFUN("SET-CLOSURE-CONST",			F_SetClosureConst,				3, 0, 0)
LFUN("CLOSURE-CODEVEC",				F_ClosureCodevec,             	1, 0, 0)
LFUN("CLOSURE-SET-DOCUMENTATION",	F_ClosureSetDocumentation,    	2, 0, 0)

LFUN("%MAKE-STRUCTURE",             F_MakeStructure,              	2, 0, 0)
LFUN("%STRUCTURE-TYPE-P",           F_StructureTypeP,             	2, 0, 0)
LFUN("COPY-STRUCTURE",				F_CopyStructure,              	1, 0, 1)
LFUN("%RECORD-REF",					F_RecordRef,					2, 0, 0)
LFUN("%RECORD-STORE",				F_RecordStore,                	3, 0, 0)
LFUN("%RECORD-LENGTH",				F_RecordLength,					1, 0, 0)

LFUN("CD",							F_CD,                         	0, 1, 3)
LFUN("PATHNAME",                    F_Pathname,                   	1, 0, 1)
LFUN("TRUENAME",                    F_Truename,                   	1, 0, 1)
LFUN("PROBE-PATHNAME",				F_ProbePathname,                1, 0, 3)
LFUN_K("MAKE-PATHNAME",             F_MakePathname,               	0, 0, 1, K_HOST K_DEVICE K_DIRECTORY K_NAME K_TYPE K_VERSION K_DEFAULTS K_CASE K_LOGICAL)
LFUN_K("PARSE-NAMESTRING",          F_ParseNamestring,           	1, 2, 1, K_START K_END K_JUNK_ALLOWED)
LFUN_K("MAKE-HASH-TABLE",           F_MakeHashTable,              	0, 0, 1, K_TEST K_SIZE K_REHASH_SIZE K_REHASH_THRESHOLD K_INITIAL_CONTENTS K_KEY_TYPE K_VALUE_TYPE K_WARN_IF_NEEDS_REHASH_AFTER_GC K_WEAK)

LFUN("MAP-EXTERNAL-SYMBOLS",		F_MapExternalSymbols,         	2, 0, 0)
//!!!R LFUN("_NORM-PAIR",					F_NormPair,                   	2, 0, 0)

LFUN("GC",                          F_GC,                         	0, 0, 3)
LFUN("FINALIZE",                    F_Finalize,                   	2, 0, 3)
LFUN("MAKE-WEAK-POINTER",           F_MakeWeakPointer,            	1, 0, 3)
LFUN("WEAK-POINTER-VALUE",          F_WeakPointerValue,           	1, 0, 3)
LFUN("_SETF_WEAK-POINTER-VALUE",    F_SetfWeakPointerValue,       	2, 0, 3)

LFUN("SET-FUNCALLABLE-INSTANCE-FUNCTION", F_SetFuncallableInstanceFunction, 2, 0, 2)
LFUN("%COPY-GENERIC-FUNCTION",      F_CopyGenericFunction,        	2, 0, 0)
LFUN("GENERIC-FUNCTION-EFFECTIVE-METHOD-FUNCTION", F_GenericFunctionEffectiveMethodFunction, 1, 0, 0)
LFUN("STD-INSTANCE-P",              F_StdInstanceP,               	1, 0, 2)
LFUN("FUNCALLABLE-INSTANCE-P",      F_FuncallableInstanceP,       	1, 0, 2)
LFUN("ALLOCATE-STD-INSTANCE",		F_AllocateStdInstance,			2, 0, 2)
LFUN("ALLOCATE-FUNCALLABLE-INSTANCE", F_AllocateFuncallableInstance,	2, 0, 2)
LFUN("ALLOCATE-METAOBJECT-INSTANCE", F_AllocateMetaobjectInstance, 	2, 0, 2)
LFUN("%CHANGE-CLASS",               F_PChangeClass,                	2, 0, 2)
LFUN("POTENTIAL-CLASS-P",           F_PotentialClassP,           	1, 0, 2)
LFUN("DEFINED-CLASS-P",				F_DefinedClassP,				1, 0, 2)
LFUN("CLASS-GETHASH",               F_ClassGethash,               	2, 0, 2)

// CLOS
LFUN("SLOT-VALUE",                  F_SlotValue,                  	2, 0, 1)
LFUN("SET-SLOT-VALUE",              F_SetSlotValue,               	3, 0, 2)
LFUN("SLOT-BOUNDP",                 F_SlotBoundP,                 	2, 0, 1)
LFUN("SLOT-MAKUNBOUND",             F_SlotMakUnbound,             	2, 0, 1)
LFUN("SLOT-EXISTS-P",				F_SlotExistsP,					2, 0, 1)
LFUN("STANDARD-INSTANCE-ACCESS",    F_StandardInstanceAccess,     	2, 0, 2)
LFUN("(SETF STANDARD-INSTANCE-ACCESS)", F_SetfStandardInstanceAccess, 3, 0, 2)
LFUN("%UNBOUND",					F_PUnbound,						0, 0, 0)
LFUN("%SLOT-VALUE-USING-CLASS",		F_PSlotValueUsingClass,			3, 0, 2)
LFUN("%SET-SLOT-VALUE-USING-CLASS",	F_PSetSlotValueUsingClass,		4, 0, 2)
LFUN("%SLOT-BOUNDP-USING-CLASS",		F_PSlotBoundpUsingClass,	3, 0, 2)
LFUN("%SLOT-MAKUNBOUND-USING-CLASS", F_PSlotMakunboundUsingClass,	3, 0, 2)

LFUN("SPECIAL-OPERATOR-P",          F_SpecialOperatorP,           	1, 0, 1)
LFUN("EXPAND-DEFTYPE",              F_ExpandDeftype,				1, 1, 0)
LFUN("_VALID-TYPE1",				F_ValidType1,					1, 0, 0)
LFUN("_VALID-TYPE",					F_ValidType,					1, 0, 0)
LFUN_K("MAKE-SEQUENCE",				F_MakeSequence,					2, 0, 1,	K_INITIAL_ELEMENT )
LFUN_K("POSITION",                  F_Position,						2, 0, 1,	K_FROM_END K_TEST K_TEST_NOT K_START K_END K_KEY)
LFUN_K("REMOVE-IF",					F_RemoveIf,						2, 0, 1,	K_FROM_END K_START K_END K_COUNT K_KEY)

LFUN_K("WRITE",						F_Write,						1, 0, 1,	K_STREAM	K_ARRAY		K_BASE			K_CASE				K_CIRCLE	K_ESCAPE	K_GENSYM	K_LENGTH
																				K_LEVEL		K_LINES		K_MISER_WIDTH	K_PPRINT_DISPATCH	K_PRETTY	K_RADIX		K_READABLY	K_RIGHT_MARGIN)

LFUN_K("WRITE-STRING",				F_WriteString,					1, 1, 1,	K_START K_END )

LFUN_K("WRITE-UNREADABLE",			F_WriteUnreadable,				3, 0, 0,	K_TYPE K_IDENTITY )
LFUN_K("WRITE-BYTE-SEQUENCE",		F_WriteByteSequence,			2, 0, 3,	K_START K_END )

LFUN("INTEGER-LENGTH",              F_IntegerLength,               	1, 0, 1)
LFUN("LOGBITP",                     F_LogBitP,                    	2, 0, 1)
LFUN("LDB",                         F_Ldb,                        	2, 0, 1)
LFUN("MASK-FIELD",					F_MaskField,                   	2, 0, 1)
LFUN("DEPOSIT-FIELD",               F_DepositField,               	3, 0, 1)

#if UCFG_LISP_FFI
LFUN("POINTER-ADDRESS",             F_PointerAddress,               1, 0, 7)
LFUN("MAKE-POINTER",             	F_MakePointer,               	1, 0, 7)
LFUN("%FOREIGN-ALLOC",              F_ForeignAlloc,                 1, 0, 7)
LFUN("FOREIGN-FREE",                F_ForeignFree,                  1, 0, 7)
LFUN("%FOREIGN-TYPE-SIZE",          F_ForeignTypeSize,              1, 0, 7)
LFUN("%FOREIGN-TYPE-ALIGNMENT",     F_ForeignTypeAlignment,         1, 0, 7)
LFUN("%MEM-REF",     				F_MemRef,         				2, 1, 7)
LFUN("%MEM-SET",     				F_MemSet,         				3, 1, 7)
LFUN("%CREATE-CALLBACK",           	F_CreateCallback,               4, 0, 7)
LFUN("%FOREIGN-SYMBOL-POINTER",		F_ForeignSymbolPointer,			2, 0, 7)
LFUN("%LOAD-FOREIGN-LIBRARY",		F_LoadForeignLibrary,			2, 0, 7)
LFUN("%CLOSE-FOREIGN-LIBRARY",		F_CloseForeignLibrary,			1, 0, 7)
#endif

LFUN("%FIND-SUBR",                  F_FindSubr,                   	1, 0, 0)
LFUN("%DEFIO",                      F_Defio,                     	2, 0, 0)

LFUN("USER-HOMEDIR-PATHNAME",       F_UserHomedirPathname,        	0, 1, 1)
LFUN("GETENV",						F_Getenv,                     	0, 1, 3)

LFUN("MACHINE-INSTANCE",			F_MachineInstance,				0, 0, 1)
LFUN("MACHINE-TYPE",				F_MachineType,					0, 0, 1)
LFUN("MACHINE-VERSION",				F_MachineVersion,				0, 0, 1)
LFUN("SOFTWARE-TYPE",				F_SoftwareType,					0, 0, 1)                                                                  
LFUN("SOFTWARE-VERSION",            F_SoftwareVersion,				0, 0, 1)                                                                  
LFUN("LISP-IMPLEMENTATION-VERSION", F_LispImplementationVersion,  	0, 0, 1)
LFUN("VERSION",                     F_Version,                    	0, 1, 0)
LFUN("SAVEMEM",                     F_SaveMem,                    	1, 0, 0)
LFUN("SLEEP",                      	F_Sleep,                       	1, 0, 1)
LFUN("SHELL",                  		F_Shell,                   		0, 1, 3)
LFUN("EXIT",                        F_Exit,                       	0, 1, 3)

#if UCFG_LISP_DEBUGGER
LFUN("DRIVER",                      F_Driver,                     	1, 0, 0)    
LFUN("_BREAK-DRIVER",               F_BreakDriver,                	1, 0, 0)
LFUN("UNWIND-TO-DRIVER",			F_UnwindToDriver,             	1, 0, 0)    
LFUN("RETURN-FROM-EVAL-FRAME",      F_ReturnFromEvalFrame,        	2, 0, 0)
LFUN("_DESCRIBE-FRAME0",            F_DescribeFrame0,             	2, 0, 0) //!!!
LFUN("_FRAME-INFO",                 F_FrameInfo,                  	0, 1, 0)
LFUN("FRAME-UP-1",                  F_FrameUp1,                   	2, 0, 0)
LFUN("FRAME-DOWN-1",                F_FrameDown1,                 	2, 0, 0)
LFUN("THE-FRAME",                   F_TheFrame,                   	0, 0, 0)
LFUN("DRIVER-FRAME-P",              F_DriverFrameP,               	1, 0, 0)
LFUN("EVAL-FRAME-P",                F_EvalFrameP,                 	1, 0, 0)
LFUN("DESCRIBE-FRAME",				F_DescribeFrame,				2, 0, 0)
LFUN("SHOW-STACK",					F_ShowStack,					0, 3, 3)
LFUN("SAME-ENV-AS",                 F_SameEnvAs,                  	2, 0, 0)
#endif // UCFG_LISP_DEBUGGER

#ifndef C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR
LFUN("BQ-EXPAND",					F_BqExpand,                   	1, 0, 0)
#endif

LFUN("_DEFAULT-BREAK-DRIVER",      	F_DefaultBreakDriver,           1, 2, 0)
LFUN("_OBFUSCATE",                  F_Obfuscate,                  	1, 0, 0)
//!!!R LFUN("_EMPTY",                      F_Empty,                      	0, 0, 0)

LFUN_END

LFUN_BEGIN_R

LFUNR("FUNCALL",                    F_Funcall,                    	1, 0, 1)
LFUNR("APPLY",                      F_Apply,                      	2, 0, 1)
LFUNR("LIST",                       F_List,                       	0, 0, 1)
LFUNR("LIST*",                      F_ListEx,                     	1, 0, 1)
LFUNR("APPEND",                     F_Append,                     	0, 0, 1)
LFUNR("CONCATENATE",				F_Concatenate,					1, 0, 1)
LFUNR("NCONC",                      F_Nconc,                      	0, 0, 1)
LFUNR("ARRAY-ROW-MAJOR-INDEX",      F_ArrayRowMajorIndex,         	1, 0, 1)
LFUNR("VALUES",                     F_Values,                     	0, 0, 1)
//!!!R LFUNR("%CONCS",                     F_ConcS,                      0, 0, 0)                                                                  
//!!!R LFUNR("STRING-CONCAT",              F_StringConcat,               0, 0, 3)
LFUNR("MAKE-BROADCAST-STREAM",      F_MakeBroadcastStream,        	0, 0, 1)
LFUNR("MAKE-CONCATENATED-STREAM",   F_MakeConcatenatedStream,     	0, 0, 1)

LFUNR("+",							F_Plus,                       	0, 0, 1)
LFUNR("-",							F_Minus,                      	1, 0, 1)
LFUNR("*",							F_Multiply,                     0, 0, 1)
LFUNR("/",							F_Divide,                     	1, 0, 1)
LFUNR("GCD",						F_GCD,                     		0, 0, 1)
LFUNR("<",							F_Lesser,                     	1, 0, 1)
LFUNR(">",							F_Greater,                    	1, 0, 1)
LFUNR(">=",							F_GreaterOrEqual,             	1, 0, 1)
LFUNR("<=",							F_LesserOrEqual,              	1, 0, 1)
LFUNR("=",							F_EqNum,                      	1, 0, 1)
LFUNR("_/=",						F_AllDifferent,					2, 0, 0)
LFUNR("/=",							F_NotEqNum,                   	1, 0, 1)

LFUNR("LOGIOR",                     F_LogIOR,                     	0, 0, 1)
LFUNR("LOGXOR",						F_LogXOR,                     	0, 0, 1)
LFUNR("LOGAND",                     F_LogAND,                     	0, 0, 1)
LFUNR("LOGEQV",                     F_LogEQV,                     	0, 0, 1)


LFUNR("VECTOR",                     F_Vector,                     	0, 0, 1)
LFUNR("%ALLOCATE-INSTANCE",         F_PAllocateInstance,          	1, 0, 2)
LFUNR("%MAKE-INSTANCE",				F_PMakeInstance,		        1, 0, 2)
LFUNR("CLASS-TUPLE-GETHASH",        F_ClassTupleGethash,          	2, 0, 2)
LFUNR("MAPCAR",                     F_MapCar,                     	2, 0, 1)
LFUNR("MAPLIST",                    F_MapList,                    	2, 0, 1)
LFUNR("MAPCAN",						F_MapCan,						2, 0, 1)
LFUNR("MAPCON",						F_MapCon,						2, 0, 1)
LFUNR("MAPC",						F_MapC,							2, 0, 1)
LFUNR("MAPL",						F_MapL,							2, 0, 1)

LFUNR("MAP-INTO",					F_MapInto,						2, 0, 1)

//!!!    LFUNR("SIGNAL",                      F_Signal,                    1, 0, 1),

LFUNR("AREF",                       F_Aref,							1, 0, 1)
LFUNR("STORE",                      F_Store,						2, 0, 0)
//!!!R LFUNR("_EMPTYR",                    F_EmptyR,                     	0, 0, 0)
LFUNR("_PR",						F_Pr,                         	0, 0, 0)

#if UCFG_LISP_FFI
LFUNR("_%FOREIGN-FUNCALL",			F_ForeignFuncall,				3, 0, 7)
#endif

LFUN_END_R


LFUN_BEGIN_SO

LSO("IF",                          F_If,                         	2, 1, 0, 1)
LSO("SETQ",                        F_Setq,                       	0, 0, 1, 1)
LSO("QUOTE",                       F_Quote,                      	1, 0, 0, 1)
LSO("LET",                         F_Let,                        	1, 0, 1, 1)
LSO("LET*",                        F_LetA,                       	1, 0, 1, 1)
LSO("COMPILER-LET",                F_CompilerLet,                	1, 0, 1, 3)
LSO("GO",                          F_Go,                         	1, 0, 0, 1)
LSO("BLOCK",                       F_Block,                      	1, 0, 1, 1)
LSO("FUNCTION",                    F_Function,                   	1, 1, 0, 1)
LSO("PROGN",                       F_Progn,                      	0, 0, 1, 1)
LSO("THE",                         F_The,                        	2, 0, 0, 1)
LSO("TAGBODY",                     F_Tagbody,                    	0, 0, 1, 1)
LSO("RETURN-FROM",                 F_ReturnFrom,                 	1, 1, 0, 1)
LSO("FLET",                        F_Flet,                       	1, 0, 1, 1)
LSO("FUNCTION-MACRO-LET",          F_FunctionMacroLet,           	1, 0, 1, 0)
LSO("LABELS",                      F_Labels,                     	1, 0, 1, 1)
LSO("EVAL-WHEN",                   F_EvalWhen,                   	1, 0, 1, 1)
LSO("LOAD-TIME-VALUE",             F_LoadTimeValue,              	1, 1, 0, 1)
LSO("LOCALLY",                     F_Locally,                    	0, 0, 1, 1)
LSO("MACROLET",                    F_Macrolet,                   	1, 0, 1, 1)
LSO("MULTIPLE-VALUE-CALL",         F_MultipleValueCall,          	1, 0, 1, 1)
LSO("MULTIPLE-VALUE-PROG1",        F_MultipleValueProg1,         	1, 0, 1, 1)
LSO("PROGV",                       F_Progv,                      	2, 0, 1, 1)
LSO("SYMBOL-MACROLET",             F_SymbolMacrolet,             	1, 0, 1, 1)
LSO("THROW",                       F_Throw,                      	2, 0, 0, 1)
LSO("CATCH",                       F_Catch,                      	1, 0, 1, 1)
LSO("UNWIND-PROTECT",              F_UnwindProtect,              	1, 0, 1, 1)

#if UCFG_LISP_MACRO_AS_SPECIAL_OPERATOR
LSO("COND",                        F_Cond,                       	0, 0, 1, 1)
LSO("CASE",                        F_Case,                       	1, 0, 1, 1)
LSO("AND",                         F_And,                        	0, 0, 1, 1)
LSO("OR",                          F_Or,                         	0, 0, 1, 1)
LSO("MULTIPLE-VALUE-BIND",         F_MultipleValueBind,          	2, 0, 1, 1)
LSO("PSETQ",                       F_PSetq,                      	0, 0, 1, 1)
#endif
#ifdef C_LISP_BACKQUOTE_AS_SPECIAL_OPERATOR
LSO("BACKQUOTE",                   F_Backquote,                  	1, 0, 0, 0)
#endif

LSO("DECLARE",                     F_Declare,                    	0, 0, 1, 1)

LFUN_END_SO
