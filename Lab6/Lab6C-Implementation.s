// void CopyCell(uint32_t *dst, uint32_t *src) ;        // R0 = dst, R1 = src

            .syntax     unified
            .cpu        cortex-m4
            .text

            .global     CopyCell
            .thumb_func
            .align
CopyCell:  
            
         PUSH {R4-R11} //variable registers
         LDR R2,=60 //counter
NextRow1:   
     //loop 1 repeat 6 time
    .rept 6
            LDMIA R1!, {R3-R12} //loading registers and increments from source
            STMIA R0!, {R3-R12} //storing registers in to destination
    .endr
            ADD R0,R0,720 //next src row
            ADD R1,R1,720 //next dest row
           
            
            SUBS.N R2, R2, 1  // decrementing counter


            BNE         NextRow1 //branch


EndRows1:   //Implementation
            POP {R4-R11}  //pop variable registers 
            BX          LR


// void FillCell(uint32_t *dst, uint32_t color) ;       // R0 = dst, R1 = color

            .global     FillCell
            .thumb_func
            .align 
FillCell:   LDR R2,=60 //counter 

NextRow2:   LDR R3, =60/2 //counter for row, moving by 2 pizels

NextCol2:

            STRD R1, R1,[R0],8 //store 2 pixels 
            SUBS.N R3, R3, 1  // decrementing COLUMN counter
            BNE      NextCol2
            ADD R0,R0,720 //next dest row
            SUBS.N R2, R2, 1  // decrementing ROW counter
            BNE NextRow2
            BX          LR

            .end














