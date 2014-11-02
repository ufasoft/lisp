/*######     Copyright (c) 1997-2012 Ufasoft  http://ufasoft.com  mailto:support@ufasoft.com    ##########################################
# This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published #
# by the Free Software Foundation; either version 3, or (at your option) any later version. This program is distributed in the hope that #
# it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. #
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this #
# program; If not, see <http://www.gnu.org/licenses/>                                                                                    #
########################################################################################################################################*/
#include <el/ext.h>
using namespace Ext;

#include "resource.h"

CStringResEntry s_arStringRes[] = {
	IDS_E_ObjectCannotStart, 	"an object cannot start with ~S",
	IDS_E_FileClosed, 			"File ~S if closed",
	IDS_E_DivideByZero, 		"divide by zero",
	IDS_E_FunctionUndefined, 	"function undefined ~S",
	IDS_E_IsNot, 				"~S is not a ~S",
	IDS_E_ParseError, 			"Parse error",
	IDS_E_ImportConflict, 		"Importing ~S into ~S produces a name conflict with ~S and other symbols.",
	IDS_E_UninterNameConflict, 	"Uninterning ~S from ~S uncovers a name conflict.\nYou may choose the symbol in favour of which to resolve the conflict.",
	IDS_E_ImportBeforeExport, 	"~S: Symbol ~S should be imported into ~S before being exported.",
	IDS_E_ExportConflict, 		"Exporting ~S from ~S produces a name conflict with ~S from ~S.\nYou may choose which symbol should be accessible in ~S.",
	IDS_E_NoPackageWithName, 	"there is no package with name ~S",
	IDS_E_NoExternalSymbol, 	"~S has no external symbol with name ~S",
	IDS_E_NoSuchBlock, 			"no block named ~A is currently visible",
	IDS_E_NoSuchThrow, 			"there is no CATCH for tag ~S",
	IDS_E_BlockHasLeft, 		"the block named ~S has already been left",
	IDS_E_TooFewArguments, 		"too few arguments given to ~A: ~S",
	IDS_E_VariableHasNoValue, 	"variable ~S has no value",
	IDS_E_IsNotAFunctionName, 	"~S: ~S is not a function name",
	IDS_E_NaN, 					"~S: Not a number result",
	IDS_E_TooManyValues, 		"~S: too many values",
	IDS_E_KeywordArgsNoPairwise, "keyword arguments in ~S should occur pairwise",
	IDS_E_NoSuchOpcode, 		"~S: no such opcode ~S",
	IDS_E_SeriousCondition, 	"~S: unrecoverable internal error",
};

static int InitStringRes() {
	for (int i=0; i<_countof(s_arStringRes); ++i)
		MapStringRes()[s_arStringRes[i].ID] = s_arStringRes[i].Ptr;
	return 1;
}

static int s_initStringRes = InitStringRes();

