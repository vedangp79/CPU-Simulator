        lw      0       5       a  
        lw      0       1       b
        lw      0       7       c
        add     0       1       2       goto end of program when reg1==0
        beq     0       0       32   go back to the beginning of the loop
c       sw      0       6       d
        lw      0       4       5
        jalr    4       5               end of program
        beq     3       3       -2      go back to the beginning of the loop
d       .fill   1
b       .fill   c
a       .fill   9
