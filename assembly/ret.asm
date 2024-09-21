SET R0 = main
SET R5 = hello_str
CALL *R0
OUT R5

main:
    RET

hello_str:
    STRING "H"