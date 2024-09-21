SET R5 = hello_str
CALL main
HALT

main: OUT R5

hello_str:
    STRING "H"