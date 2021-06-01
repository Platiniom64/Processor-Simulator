/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or online)|
|   2. Fill mipssim.c                                                                 |
|   3. Do not use any other .c files neither alter mipssim.h or parser.h              |
|   4. Do not include any other library files                                         |
|*************************************************************************************/

#include "mipssim.h"

#define BREAK_POINT 200000 // exit after so many cycles -- useful for debugging

#define I_TYPE 2
#define J_TYPE 3

// Global variables
char mem_init_path[1000];
char reg_init_path[1000];

uint32_t cache_size = 0;
struct architectural_state arch_state;
//comment

// * get the instruction type based on the opcode
// * COMPLETE
static inline uint8_t get_instruction_type(int opcode)
{
    // the first instruction types are defined in mipssim.h
    //  R_TYPE = 1
    // int I_TYPE = 2;
    //int J_TYPE = 3;
    
    int FR_TYPE = 4; // ignored
    int FI_TYPE = 5; // ignored
    //  EOP    = 6

    switch (opcode) {

        ///@students: fill in the rest

        // all R-type instructions
        case SPECIAL: 
            return R_TYPE;

        // all immediate instructions
        case 35: // lw
        case 43: // sw
        case 8:  //addi
        case 4:  // beq
            return I_TYPE;

        // jump instruction
        case 2: 
            return J_TYPE;

        case EOP:    // 111111
            return EOP_TYPE;
        
        default:
            assert(false);
    }
    assert(false);
}

// * sets the control wires for each state
// * COMPLETE
void FSM()
{
    struct ctrl_signals* control = &arch_state.control;
    struct instr_meta* IR_meta = &arch_state.IR_meta;

    //reset control signals
    memset(control, 0, (sizeof(struct ctrl_signals)));

    int opcode = IR_meta->opcode;
    int state = arch_state.state;
    switch (state) {

        case INSTR_FETCH:
            control->MemRead = 1;
            control->ALUSrcA = 0;
            control->IorD = 0;
            control->IRWrite = 1;
            control->ALUSrcB = 1;
            control->ALUOp = 0;
            control->PCWrite = 1;
            control->PCSource = 0;
            state = DECODE;
            break;
        case DECODE:
            control->ALUSrcA = 0;
            control->ALUSrcB = 3;
            control->ALUOp = 0;
            // next state depends on the instruction
            switch (opcode) {
                case SPECIAL:
                    state = EXEC;
                    break;
                case J:
                    state = JUMP_COMPL;
                    break;
                case BEQ:
                    state = BRANCH_COMPL;
                    break;
                case LW:
                case SW:
                    state = MEM_ADDR_COMP;
                    break;
                case ADDI:
                    state = I_TYPE_EXEC;
                    break;
                case EOP:
                    state = EXIT_STATE;
                    break;
                default:
                    printf("opcode not recognised for FSM\n");
                    assert(false);
            }
            break;

        case JUMP_COMPL:
            control->PCWrite = 1;
            control->PCSource = 2;
            state = INSTR_FETCH;
            break;

        case BRANCH_COMPL:
            control->ALUSrcA = 1;
            control->ALUSrcB = 0;
            control->ALUOp = 1;
            control->PCWriteCond = 1;
            control->PCSource = 1;
            state = INSTR_FETCH;
            break;

        case MEM_ADDR_COMP:
            control->ALUSrcA = 1;
            control->ALUSrcB = 2;
            control->ALUOp = 0;
            // next state depends on the opcode!
            if (opcode == SW) state = MEM_ACCESS_ST;
            else if (opcode == LW) state = MEM_ACCESS_LD;
            else assert(false);
            break;
        case MEM_ACCESS_ST: // store
            control->MemWrite = 1;
            control->IorD = 1;
            state = INSTR_FETCH;
            break;
        case MEM_ACCESS_LD: // load
            control->MemRead = 1;
            control->IorD = 1;
            state = WB_STEP;
            break;
        case WB_STEP: // finish load
            control->RegDst = 0;
            control->RegWrite = 1;
            control->MemtoReg = 1;
            state = INSTR_FETCH;
            break;


        case EXEC:
            control->ALUSrcA = 1;
            control->ALUSrcB = 0;
            control->ALUOp = 2;
            state = R_TYPE_COMPL;
            break;
        case R_TYPE_COMPL:
            control->RegDst = 1;
            control->RegWrite = 1;
            control->MemtoReg = 0;
            state = INSTR_FETCH;
            break;

        case I_TYPE_EXEC:
            control->ALUSrcA = 1;
            control->ALUSrcB = 2;
            control->ALUOp = 0;
            state = I_TYPE_COMPL;
            break;
        case I_TYPE_COMPL:
            control->RegDst = 0;
            control->RegWrite = 1;
            control->MemtoReg = 0;
            state = INSTR_FETCH;
            break;

        default: assert(false);
    }
    arch_state.state = state;
}

// * COMPLETE
void instruction_fetch()
{
    if ((arch_state.control.MemRead == 1) && (arch_state.control.IorD == 0)) {
        int address = arch_state.curr_pipe_regs.pc;
        int content = memory_read(address);

        if (arch_state.control.IRWrite) {
            arch_state.next_pipe_regs.IR = content;
        }
        
        arch_state.next_pipe_regs.MDR = content;
    }
}

// * COMPLETE
void decode_and_read_RF()
{
    int read_register_1 = arch_state.IR_meta.reg_21_25;
    int read_register_2 = arch_state.IR_meta.reg_16_20;
    check_is_valid_reg_id(read_register_1);
    check_is_valid_reg_id(read_register_2);
    arch_state.next_pipe_regs.A = arch_state.registers[read_register_1];
    arch_state.next_pipe_regs.B = arch_state.registers[read_register_2];
}

// * COMPLETE
void execute()
{
    struct ctrl_signals* control = &arch_state.control;
    struct instr_meta* IR_meta = &arch_state.IR_meta;
    struct pipe_regs* curr_pipe_regs = &arch_state.curr_pipe_regs;
    struct pipe_regs* next_pipe_regs = &arch_state.next_pipe_regs;

    int alu_opA = (control->ALUSrcA == 1) ? curr_pipe_regs->A : curr_pipe_regs->pc;
    int alu_opB = 0;
    int immediate = IR_meta->immediate;
    int shifted_immediate = (IR_meta->immediate) << 2;
    int jump_address = (get_piece_of_a_word(curr_pipe_regs->pc, 27, 4) << 28) + (IR_meta->jmp_offset << 2);


    // * this chooses the second operand for the ALU based on ALUSrcB
    // * COMPLETED
    switch (control->ALUSrcB) {
        case 0:
            alu_opB = curr_pipe_regs->B;
            break;
        case 1:
            alu_opB = WORD_SIZE;
            break;
        case 2:
            alu_opB = immediate;
            break;
        case 3: // for a branch
            alu_opB = shifted_immediate;
            break;
        default:
            assert(false);
    }

    // * this controls what is the job of the ALU (hence what is its output) based on ALUop from control
    // * see page 317
    // * COMPLETE
    switch (control->ALUOp) {
        // load and store word, calculate destination address based on base address and offset
        // for ADDI you add the register with the immediate
        // for a branch in cycle 2 when we calculate the branch
        case 0:
            next_pipe_regs->ALUOut = alu_opA + alu_opB;
            break;

        // for branch equal
        case 1:
            next_pipe_regs->ALUOut = alu_opA - alu_opB; // check if regiters are the same
            break;
        
        // for R type instructions
        case 2:
            switch (IR_meta->function) {

                case ADD:
                    next_pipe_regs->ALUOut = alu_opA + alu_opB;
                    break;
                
                // for ADDU, you should ignore the overflow
                case 33: 
                    next_pipe_regs->ALUOut = (int) (((long) alu_opA) + ((long) alu_opB));
                    break;

                case SLT:
                    next_pipe_regs->ALUOut = (alu_opA < alu_opB) ? 1 : 0;
                    break;

                default:
                    printf("Not implemented the function field for ALUop value of 2 (R type)\n");
                    assert(false);
            }
            break;

        default:
            printf("Not implemented the value of ALUop\n");
            assert(false);
    }

    // * PC calculation
    // * COMPLETE
    if (control->PCWrite || (control->PCWriteCond && (next_pipe_regs->ALUOut == 0))) {
        switch (control->PCSource) {
            // comes from directly the ALU, when we do the instruction fetch fase (we added 4 to PC)
            case 0:
                check_address_is_word_aligned(next_pipe_regs->ALUOut);
                next_pipe_regs->pc = next_pipe_regs->ALUOut;
                break;
            
            // this is the address calculated when we had a branch
            case 1:
                check_address_is_word_aligned(curr_pipe_regs->ALUOut);
                next_pipe_regs->pc = curr_pipe_regs->ALUOut; // in curr as address was calculated in decode cycle
                break;

            // the new address for a jump
            case 2:
                check_address_is_word_aligned(jump_address);
                next_pipe_regs->pc = jump_address;
                break;

            default:
                printf("Wrong value for PCSource \n");
                assert(false);
        }
    }
}

// * this method will read or write data into the Memory, for a sw and lw only
// * COMPLETE
void memory_access() {
    ///@students: appropriate calls to functions defined in memory_hierarchy.c must be added

    struct ctrl_signals* control = &arch_state.control;
    struct pipe_regs* curr_pipe_regs = &arch_state.curr_pipe_regs;
    struct pipe_regs* next_pipe_regs = &arch_state.next_pipe_regs;
    
    // only execute if we are executing a lw or sw 
    if (control->IorD) {

        // regardless if it is write or read
        int address = curr_pipe_regs->ALUOut;
        check_address_is_word_aligned(address); // check that the address to the memory is valid

        // * for a load word
        if (control->MemRead) {
            // read the data
            int read_data = memory_read(address);
            // store it in the memory data register
            next_pipe_regs->MDR = read_data;
        }

        // * for a store word
        if (control->MemWrite) {
            // find what to store
            int write_data = curr_pipe_regs->B;
            // write it in memory
            memory_write(address, write_data);
        }
    }
}


// * this method writes to the register file, for graph state 4, 7 and I_complete
// * COMPLETE
void write_back()
{
    if (arch_state.control.RegWrite) {

        struct ctrl_signals* control = &arch_state.control;
        
        // * find where the data must be written (which register id)
        // * COMPLETE
        int write_reg_id;

        switch (control->RegDst) {
            // for a lw get the word into the rt gegister
            case 0:
                write_reg_id = arch_state.IR_meta.reg_16_20;
                check_is_valid_reg_id(write_reg_id);
                break;

            // for a normal R-type instruction
            case 1:
                write_reg_id =  arch_state.IR_meta.reg_11_15;
                check_is_valid_reg_id(write_reg_id);
                break;

            default:
                printf("RegDst not valid, must be 0 or 1\n");
                assert(false);
        }

        // * find what data to write
        // * COMPLETE
        int write_data;
        switch (control->MemtoReg) {
            // comes from the AlUout, so any R type such as adding
            case 0:
                write_data =  arch_state.curr_pipe_regs.ALUOut;
                break;

            // for a load word instruction
            case 1:
                write_data =  arch_state.curr_pipe_regs.MDR;
                break;
            
            default:
                printf("MemToReg not valid, must be 0 or 1\n");
                assert(false);
        }

        // * actually write the data in register file now
        if (write_reg_id > 0) {
            arch_state.registers[write_reg_id] = write_data;
            printf("Reg $%u = %d \n", write_reg_id, write_data);
        } else printf("Attempting to write reg_0. That is likely a mistake \n");

    }
}

// * set up the instruction register
// * COMPLETE
void set_up_IR_meta(int IR, struct instr_meta *IR_meta)
{
    IR_meta->opcode = get_piece_of_a_word(IR, OPCODE_OFFSET, OPCODE_SIZE);
    IR_meta->immediate = get_sign_extended_imm_id(IR, IMMEDIATE_OFFSET);
    IR_meta->function = get_piece_of_a_word(IR, 0, 6);
    IR_meta->jmp_offset = get_piece_of_a_word(IR, 0, 26);
    IR_meta->reg_11_15 = (uint8_t) get_piece_of_a_word(IR, 11, REGISTER_ID_SIZE);
    IR_meta->reg_16_20 = (uint8_t) get_piece_of_a_word(IR, 16, REGISTER_ID_SIZE);
    IR_meta->reg_21_25 = (uint8_t) get_piece_of_a_word(IR, 21, REGISTER_ID_SIZE);
    IR_meta->type = get_instruction_type(IR_meta->opcode);

    switch (IR_meta->opcode) {

        case SPECIAL: // R-type

            switch (IR_meta->function) {
                case ADD:
                    printf("Executing ADD(%d), $%u = $%u + $%u (function: %u) \n",
                       IR_meta->opcode,  IR_meta->reg_11_15, IR_meta->reg_21_25,  IR_meta->reg_16_20, IR_meta->function);
                    break;
                
                case 33: //addu
                    printf("Executing ADDU(%d), $%u = $%u + $%u (function: %u) \n",
                       IR_meta->opcode,  IR_meta->reg_11_15, IR_meta->reg_21_25,  IR_meta->reg_16_20, IR_meta->function);
                    break;
                
                case SLT:
                    printf("Executing SLT(%d), $%u = $%u < $%u ? (function: %u) \n",
                       IR_meta->opcode,  IR_meta->reg_11_15, IR_meta->reg_21_25,  IR_meta->reg_16_20, IR_meta->function);
                    break;
                
                default:
                    printf("cound't find the fuction code to print the instruction");
                    assert(false);
                    break;
            }
            break;
        
        case LW:
            printf("Executing LW(%d), $%u = offset at address in $%u with immediate %d \n",
                       IR_meta->opcode,  IR_meta->reg_16_20,  IR_meta->reg_21_25, IR_meta->immediate);
            break;
        case SW:
            printf("Executing SW(%d), $%u goes to memory at address in $%u with offset %d \n",
                       IR_meta->opcode,  IR_meta->reg_16_20,  IR_meta->reg_21_25, IR_meta->immediate);
            break;

        case ADDI:
            printf("Executing ADDI(%d), $%u = $%u + immediate %d \n",
                       IR_meta->opcode,  IR_meta->reg_16_20,  IR_meta->reg_21_25, IR_meta->immediate);
            break;

        case BEQ:
            printf("Executing BEQ(%d), if ($%u == $%u) then next line to execute: %d \n",
                       IR_meta->opcode,  IR_meta->reg_16_20,  IR_meta->reg_21_25, IR_meta->immediate + 1);
            break;

        case J:
            printf("Executing Jump(%d), PC = immediate from jump not calculated: %d \n",
                       IR_meta->opcode,  IR_meta->jmp_offset);
            break;



        case EOP:
            printf("Executing EOP(%d) \n", IR_meta->opcode);
            break;

        default: assert(false);
    }
}


// * COMPLETE
void assign_pipeline_registers_for_the_next_cycle()
{
    struct ctrl_signals* control = &arch_state.control;
    struct instr_meta* IR_meta = &arch_state.IR_meta;
    struct pipe_regs* curr_pipe_regs = &arch_state.curr_pipe_regs;
    struct pipe_regs* next_pipe_regs = &arch_state.next_pipe_regs;

    if (control->IRWrite) {
        curr_pipe_regs->IR = next_pipe_regs->IR;
        printf("PC %d: ", curr_pipe_regs->pc / 4);
        set_up_IR_meta(curr_pipe_regs->IR, IR_meta);
    }

    // always overwritten
    curr_pipe_regs->ALUOut = next_pipe_regs->ALUOut;

    // this works as they are always overwritten
    curr_pipe_regs->A = next_pipe_regs->A; 
    curr_pipe_regs->B = next_pipe_regs->B;

    if (control->MemRead) {
        curr_pipe_regs->MDR = next_pipe_regs->MDR;
    }
    
    if (control->PCWrite || (control->PCWriteCond && (next_pipe_regs->ALUOut == 0))) {
        check_address_is_word_aligned(next_pipe_regs->pc);
        curr_pipe_regs->pc = next_pipe_regs->pc;
    }
}


int main(int argc, const char* argv[])
{
    /*--------------------------------------
    /------- Global Variable Init ----------
    /--------------------------------------*/
    parse_arguments(argc, argv);
    arch_state_init(&arch_state);
    ///@students WARNING: Do NOT change/move/remove main's code above this point!
    while (true) {

        ///@students: Fill/modify the function bodies of the 7 functions below,
        /// Do NOT modify the main() itself, you only need to
        /// write code inside the definitions of the functions called below.


        FSM(); // * COMPLETE

        // graph state 0
        instruction_fetch(); // * COMPLETE

        // graph state 1
        decode_and_read_RF(); // * think it is complete

        // EXECUTE: have done the majority of the work I think, the only ting not done is the pc source
        // graph state 2, 6, 8, 9, addi_exe
        execute(); // * COMPLETE

        // MEMORY_ACCESS: get data from memory for a lw and sw
        // for state 3, 5
        memory_access(); // * COMPLETE

        // WRITE_BACK write the content to the register file (after ALU operation or a loadword)
        // for graph state 4, 7, I_compl
        write_back(); // * COMPLETE

        assign_pipeline_registers_for_the_next_cycle(); // * COMPLETE


       ///@students WARNING: Do NOT change/move/remove code below this point!
        marking_after_clock_cycle();
        arch_state.clock_cycle++;
        // Check exit statements
        if (arch_state.state == EXIT_STATE) { // I.E. EOP instruction!
            printf("Exiting because the exit state was reached \n");
            break;
        }
        if (arch_state.clock_cycle == BREAK_POINT) {
            printf("Exiting because the break point (%u) was reached \n", BREAK_POINT);
            break;
        }
    }

    marking_at_the_end();
}