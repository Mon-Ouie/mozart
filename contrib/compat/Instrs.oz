




































InstrArgs =
instrArgs(












'endOfFile': none(nil)

'skip': none(nil)
'failure': none(nil)

'definition': none([xRegisterIndexArg labelArg predIdArg predicateRefArg gRegRefArg ])
'definitionCopy': none([xRegisterIndexArg labelArg predIdArg predicateRefArg gRegRefArg ])
'endDefinition': none([labelArg     ])

'moveXX': two([registerArg registerArg    ] move x x)
'moveXY': two([registerArg registerArg    ] move x y)
'moveXG': two([registerArg registerArg    ] move x g)
'moveYX': two([registerArg registerArg    ] move y x)
'moveYY': two([registerArg registerArg    ] move y y)
'moveYG': two([registerArg registerArg    ] move y g)
'moveGX': two([registerArg registerArg    ] move g x)
'moveGY': two([registerArg registerArg    ] move g y)
'moveGG': two([registerArg registerArg    ] move g g)

'moveMoveXYXY': none([xRegisterIndexArg yRegisterIndexArg xRegisterIndexArg yRegisterIndexArg  ])
'moveMoveYXYX': none([yRegisterIndexArg xRegisterIndexArg yRegisterIndexArg xRegisterIndexArg  ])
'moveMoveXYYX': none([xRegisterIndexArg yRegisterIndexArg yRegisterIndexArg xRegisterIndexArg  ])
'moveMoveYXXY': none([yRegisterIndexArg xRegisterIndexArg xRegisterIndexArg yRegisterIndexArg  ])

'createVariableX': one([registerArg     ] createVariable x)
'createVariableY': one([registerArg     ] createVariable y)
'createVariableG': one([registerArg     ] createVariable g)

'createVariableMoveX': one([registerArg xRegisterIndexArg    ] createVariableMove x)
'createVariableMoveY': one([registerArg xRegisterIndexArg    ] createVariableMove y)
'createVariableMoveG': one([registerArg xRegisterIndexArg    ] createVariableMove g)

'unifyXX': two([registerArg registerArg    ] unify x x)
'unifyXY': two([registerArg registerArg    ] unify x y)
'unifyXG': two([registerArg registerArg    ] unify x g)
'unifyYX': two([registerArg registerArg    ] unify y x)
'unifyYY': two([registerArg registerArg    ] unify y y)
'unifyYG': two([registerArg registerArg    ] unify y g)
'unifyGX': two([registerArg registerArg    ] unify g x)
'unifyGY': two([registerArg registerArg    ] unify g y)
'unifyGG': two([registerArg registerArg    ] unify g g)

'putRecordX': one([valueArg recordArityArg registerArg   ] putRecord x)
'putRecordY': one([valueArg recordArityArg registerArg   ] putRecord y)
'putRecordG': one([valueArg recordArityArg registerArg   ] putRecord g)
'putListX': one([registerArg     ] putList x)
'putListY': one([registerArg     ] putList y)
'putListG': one([registerArg     ] putList g)
'putConstantX': one([valueArg registerArg    ] putConstant x)
'putConstantY': one([valueArg registerArg    ] putConstant y)
'putConstantG': one([valueArg registerArg    ] putConstant g)

'setVariableX': one([registerArg     ] setVariable x)
'setVariableY': one([registerArg     ] setVariable y)
'setVariableG': one([registerArg     ] setVariable g)
'setValueX': one([registerArg     ] setValue x)
'setValueY': one([registerArg     ] setValue y)
'setValueG': one([registerArg     ] setValue g)
'setConstant': none([valueArg     ])
'setPredicateRef': none([predicateRefArg     ])
'setVoid': none([integerArg     ])

'getRecordX': one([valueArg recordArityArg registerArg   ] getRecord x)
'getRecordY': one([valueArg recordArityArg registerArg   ] getRecord y)
'getRecordG': one([valueArg recordArityArg registerArg   ] getRecord g)
'getListX': one([registerArg     ] getList x)
'getListY': one([registerArg     ] getList y)
'getListG': one([registerArg     ] getList g)
'getListValVarX': one([xRegisterIndexArg registerArg xRegisterIndexArg   ] getListValVar x)
'getListValVarY': one([xRegisterIndexArg registerArg xRegisterIndexArg   ] getListValVar y)
'getListValVarG': one([xRegisterIndexArg registerArg xRegisterIndexArg   ] getListValVar g)
'unifyVariableX': one([registerArg     ] unifyVariable x)
'unifyVariableY': one([registerArg     ] unifyVariable y)
'unifyVariableG': one([registerArg     ] unifyVariable g)
'unifyValueX': one([registerArg     ] unifyValue x)
'unifyValueY': one([registerArg     ] unifyValue y)
'unifyValueG': one([registerArg     ] unifyValue g)
'unifyValVarXX': two([registerArg registerArg    ] unifyValVar x x)
'unifyValVarXY': two([registerArg registerArg    ] unifyValVar x y)
'unifyValVarXG': two([registerArg registerArg    ] unifyValVar x g)
'unifyValVarYX': two([registerArg registerArg    ] unifyValVar y x)
'unifyValVarYY': two([registerArg registerArg    ] unifyValVar y y)
'unifyValVarYG': two([registerArg registerArg    ] unifyValVar y g)
'unifyValVarGX': two([registerArg registerArg    ] unifyValVar g x)
'unifyValVarGY': two([registerArg registerArg    ] unifyValVar g y)
'unifyValVarGG': two([registerArg registerArg    ] unifyValVar g g)
'unifyNumber': none([valueArg     ])
'unifyLiteral': none([valueArg     ])
'unifyVoid': none([integerArg     ])

'getLiteralX': one([valueArg registerArg    ] getLiteral x)
'getLiteralY': one([valueArg registerArg    ] getLiteral y)
'getLiteralG': one([valueArg registerArg    ] getLiteral g)
'getNumberX': one([valueArg registerArg    ] getNumber x)
'getNumberY': one([valueArg registerArg    ] getNumber y)
'getNumberG': one([valueArg registerArg    ] getNumber g)

'allocateL': none([integerArg     ])
'allocateL1': none(nil)
'allocateL2': none(nil)
'allocateL3': none(nil)
'allocateL4': none(nil)
'allocateL5': none(nil)
'allocateL6': none(nil)
'allocateL7': none(nil)
'allocateL8': none(nil)
'allocateL9': none(nil)
'allocateL10': none(nil)

'deAllocateL': none(nil)
'deAllocateL1': none(nil)
'deAllocateL2': none(nil)
'deAllocateL3': none(nil)
'deAllocateL4': none(nil)
'deAllocateL5': none(nil)
'deAllocateL6': none(nil)
'deAllocateL7': none(nil)
'deAllocateL8': none(nil)
'deAllocateL9': none(nil)
'deAllocateL10': none(nil)


'genCall': none([genCallInfoArg integerArg    ])
'callX': one([registerArg integerArg    ] call x)
'callY': one([registerArg integerArg    ] call y)
'callG': one([registerArg integerArg    ] call g)
'tailCallX': one([registerArg integerArg    ] tailCall x)
'tailCallY': one([registerArg integerArg    ] tailCall y)
'tailCallG': one([registerArg integerArg    ] tailCall g)

'marshalledFastCall': none([valueArg integerArg    ])

'genFastCall': none([predicateRefArg integerArg    ])

'fastCall': none([predicateRefArg integerArg    ])
'fastTailCall': none([predicateRefArg integerArg    ])

'sendMsgX': one([valueArg registerArg recordArityArg   ] sendMsg x)
'sendMsgY': one([valueArg registerArg recordArityArg   ] sendMsg y)
'sendMsgG': one([valueArg registerArg recordArityArg   ] sendMsg g)
'tailSendMsgX': one([valueArg registerArg recordArityArg   ] tailSendMsg x)
'tailSendMsgY': one([valueArg registerArg recordArityArg   ] tailSendMsg y)
'tailSendMsgG': one([valueArg registerArg recordArityArg   ] tailSendMsg g)

'applMethX': one([integerArg registerArg    ] applMeth x)
'applMethY': one([integerArg registerArg    ] applMeth y)
'applMethG': one([integerArg registerArg    ] applMeth g)
'tailApplMethX': one([integerArg registerArg    ] tailApplMeth x)
'tailApplMethY': one([integerArg registerArg    ] tailApplMeth y)
'tailApplMethG': one([integerArg registerArg    ] tailApplMeth g)

'getSelf': none([xRegisterIndexArg     ])
'setSelf': none([xRegisterIndexArg     ])
'lockThread': none([labelArg xRegisterIndexArg    ])
'inlineAt': none([valueArg xRegisterIndexArg    ])
'inlineAssign': none([valueArg xRegisterIndexArg    ])

'branch': none([labelArg     ])

'wait': none(nil)
'waitTop': none(nil)
'ask': none(nil)

'createCond': none([labelArg     ])
'createOr': none(nil)
'createEnumOr': none(nil)
'createChoice': none(nil)
'clause': none(nil)
'emptyClause': none(nil)

'thread': none([labelArg     ])

'exHandler': none([labelArg     ])
'popEx': none(nil)

'return': none(nil)
'getReturnX': one([registerArg     ] getReturn x)
'getReturnY': one([registerArg     ] getReturn y)
'getReturnG': one([registerArg     ] getReturn g)
'funReturnX': one([registerArg     ] funReturn x)
'funReturnY': one([registerArg     ] funReturn y)
'funReturnG': one([registerArg     ] funReturn g)

'nextClause': none([labelArg     ])
'lastClause': none(nil)

'shallowGuard': none([labelArg     ])
'shallowThen': none(nil)

'testLiteralX': one([registerArg valueArg labelArg   ] testLiteral x)
'testLiteralY': one([registerArg valueArg labelArg   ] testLiteral y)
'testLiteralG': one([registerArg valueArg labelArg   ] testLiteral g)
'testNumberX': one([registerArg valueArg labelArg   ] testNumber x)
'testNumberY': one([registerArg valueArg labelArg   ] testNumber y)
'testNumberG': one([registerArg valueArg labelArg   ] testNumber g)

'testRecordX': one([registerArg valueArg recordArityArg labelArg  ] testRecord x)
'testRecordY': one([registerArg valueArg recordArityArg labelArg  ] testRecord y)
'testRecordG': one([registerArg valueArg recordArityArg labelArg  ] testRecord g)
'testListX': one([registerArg labelArg    ] testList x)
'testListY': one([registerArg labelArg    ] testList y)
'testListG': one([registerArg labelArg    ] testList g)

'testBoolX': one([registerArg labelArg labelArg   ] testBool x)
'testBoolY': one([registerArg labelArg labelArg   ] testBool y)
'testBoolG': one([registerArg labelArg labelArg   ] testBool g)

'matchX': one([registerArg hashTableArg    ] match x)
'matchY': one([registerArg hashTableArg    ] match y)
'matchG': one([registerArg hashTableArg    ] match g)
'getVariableX': one([registerArg     ] getVariable x)
'getVariableY': one([registerArg     ] getVariable y)
'getVariableG': one([registerArg     ] getVariable g)
'getVarVarXX': two([registerArg registerArg    ] getVarVar x x)
'getVarVarXY': two([registerArg registerArg    ] getVarVar x y)
'getVarVarXG': two([registerArg registerArg    ] getVarVar x g)
'getVarVarYX': two([registerArg registerArg    ] getVarVar y x)
'getVarVarYY': two([registerArg registerArg    ] getVarVar y y)
'getVarVarYG': two([registerArg registerArg    ] getVarVar y g)
'getVarVarGX': two([registerArg registerArg    ] getVarVar g x)
'getVarVarGY': two([registerArg registerArg    ] getVarVar g y)
'getVarVarGG': two([registerArg registerArg    ] getVarVar g g)
'getVoid': none([integerArg     ])


'debugEntry': none([valueArg valueArg valueArg valueArg  ])
'debugExit': none([valueArg valueArg valueArg valueArg  ])

'globalVarname': none([valueArg     ])
'localVarname': none([valueArg     ])

'clearY': none([yRegisterIndexArg     ])
'profileProc': none(nil)


'callBI': none([builtinnameArg locationArg    ])
'inlinePlus1': none([xRegisterIndexArg xRegisterIndexArg    ])
'inlineMinus1': none([xRegisterIndexArg xRegisterIndexArg    ])
'inlinePlus': none([xRegisterIndexArg xRegisterIndexArg xRegisterIndexArg   ])
'inlineMinus': none([xRegisterIndexArg xRegisterIndexArg xRegisterIndexArg   ])
'inlineDot': none([xRegisterIndexArg valueArg xRegisterIndexArg   ])
'inlineUparrow': none([xRegisterIndexArg xRegisterIndexArg xRegisterIndexArg   ])

'testBI': none([builtinnameArg locationArg labelArg   ])
'testLT': none([xRegisterIndexArg xRegisterIndexArg xRegisterIndexArg labelArg  ])
'testLE': none([xRegisterIndexArg xRegisterIndexArg xRegisterIndexArg labelArg  ])



)
