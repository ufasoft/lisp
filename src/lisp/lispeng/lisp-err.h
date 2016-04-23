/*######   Copyright (c) 2015 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com,  Sergey Pavlov  mailto:dev@ufasoft.com ####
#                                                                                                                                     #
# 		See LICENSE for licensing information                                                                                         #
#####################################################################################################################################*/

#pragma once


namespace Lisp {

ENUM_CLASS(LispErr) {
	  InvalidSyntax = 1
	, RParExpected
	, TooFewArgs
	, SymbolExpected
	, SymbolNotDeclared
	, LabelNotFound
	, BadFunction
	, BadArgumentType
	, MisplacedCloseParen
	, InvalidDottedPair
	, IllegalSetfPlace
	, IllegalTypeSpecifier
	, CERROR
	, MaxLenOfToken
	, UnexpectedEOF
	, VariableUnbound
	, Return
	, Stop
	, Run
	, CallDoesNotMatch
	, AbsentCloseParen
	, MustBeSValue
	, InvalidLambdaList
	, UnboundSlot
	, Unwind
	, NoValue
	, NoFillPointer
	, IsNotVector
	, IsNotArray
	, IsNotStream
	, IsNegative
	, IsNotReadtable
	, IsNotHashTable
	, UnknownError
	, IsNotRandomState
	, NoPackageWithName
	, DisabledReadEval
	, IsNotRational
	, InvalidSeqType
	, IllegalTestArgument
	, NotCoercedToChar
	, UnrecognizedCharName
	, StackOverflow
	, DivisionByZero
	, InitNotFound
	, ReadLabel
	, UnhandledCondition
	, NoInitFileFound
	, NoConditionSystem
} END_ENUM_CLASS(LispErr);




const error_category& lisp_category();

inline error_code make_error_code(LispErr v) { return error_code(int(v), lisp_category()); }
inline error_condition make_error_condition(LispErr v) { return error_condition(int(v), lisp_category()); }


} // Lisp::

namespace std { template<> struct std::is_error_code_enum<Lisp::LispErr> : true_type {}; }
