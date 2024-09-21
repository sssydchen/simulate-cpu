
SET R0 = main
SET R5 = hello_str
CALL *R0

main:
    OUT R5

hello_str:
    STRING "H"
