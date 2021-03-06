/* ----------------------------------------------------------------------------

    (EN) armethyst - A simple ARM Simulator written in C++ for Computer Architecture
    teaching purposes. Free software licensed under the MIT License (see license
    below).

    (PT) armethyst - Um simulador ARM simples escrito em C++ para o ensino de
    Arquitetura de Computadores. Software livre licenciado pela MIT License
    (veja a licença, em inglês, abaixo).

    (EN) MIT LICENSE:

    Copyright 2020 André Vital Saúde

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

   ----------------------------------------------------------------------------
*/

#include "BasicCPU.h"

#include <iostream>
using namespace std;

BasicCPU::BasicCPU(Memory *memory) {
	this->memory = memory;
}

/**
 * Métodos herdados de CPU
 */
int BasicCPU::run(long startAddress)
{

	// inicia PC com o valor de startAddress
	PC = startAddress;

	// ciclo da máquina
	while ((cpuError != CPUerrorCode::NONE) && !processFinished) {
		IF();
		ID();
		if (fpOP) {
			EXF();
		} else {
			EXI();
		}
		MEM();
		WB();
	}
	
	if (cpuError) {
		return 1;
	}
	
	return 0;
};

/**
 * Busca da instrução.
 * 
 * Lê a memória de instruções no endereço PC e coloca no registrador IR.
 */
void BasicCPU::IF()
{
	IR = memory->readInstruction32(PC);
};

/**
 * Decodificação da instrução.
 * 
 * Decodifica o registrador IR, lê registradores do banco de registradores
 * e escreve em registradores auxiliares o que será usado por estágios
 * posteriores.
 *
 * Escreve A, B e ALUctrl para o estágio EXI
 * ATIVIDADE FUTURA: escreve registradores para os estágios EXF, MEM e WB.
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::ID()
{
	// TODO
	//		Acrescente os cases no switch já iniciado, para detectar o grupo
	//		APENAS PARA A INSTRUÇÃO A SEGUIR:
	//				'add w1, w1, w0'
	//		que aparece na linha 40 de isummation.S e no endereço 0x74 (0x68)
	//		de txt_isummation.o.txt.
	//
	// 		Deve-se detectar em IR o grupo da qual a instrução faz parte e
	//		chamar a função 'decodeGROUP()' para	o grupo detectado, onde GROUP
	//		é o sufixo do nome da função que decodifica as instruções daquele
	//		grupo. Para 'add w1, w1, w0' deve-se chamar 'decodeDataProcReg()'.
	
	int group = IR & 0x1E000000; // bits 28-25
	
	switch (group)
	{
		//100x Data Processing -- Immediate
		case 0x10000000: // x = 0
		case 0x12000000:// x = 1
			fpOP = false;
			return decodeDataProcImm();
			break;
		// case TODO
		case 0x0A000000: 
		case 0x1A000000:
			fpOP = false;
			return decodeDataProcReg();
			break;
		// x101 Data Processing -- Register on page C4-278
		case 0x1C000000: //0001 1100 ....
		case 0x0C000000: //0000 1100 ....
		case 0x08000000: //0000 1000 ....
		case 0x18000000: //0001 1000 ....
			fpOP = false;
			return decodeLoadStore();
			break;
			// 101x Loads and Stores on page C4-237
		case 0x14000000: //0001 0100 ....
		case 0x16000000: //0001 0110 ....
			fpOP = false;
			return decodeBranches();
			break;

		default:
			return 1; // instrução não implementada
	}
};

/**
 * Decodifica instruções do grupo
 * 		100x Data Processing -- Immediate
 *
 * C4.1.2 Data Processing -- Immediate (p. 232)
 * This section describes the encoding of the Data Processing -- Immediate group.
 * The encodings in this section are decoded from A64 instruction set encoding.
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::decodeDataProcImm() {
	unsigned int n, d;
	int imm;
	
	/* Add/subtract (immediate) (pp. 233-234)
		This section describes the encoding of the Add/subtract (immediate)
		instruction class. The encodings in this section are decoded from
		Data Processing -- Immediate on page C4-232.
	*/
	switch (IR & 0xFF800000)
	{
		case 0xD1000000:
			//1 1 0 SUB (immediate) - 64-bit variant on page C6-1199
			
			if (IR & 0x00400000) return 1; // sh = 1 não implementado
			
			// ler A e B
			n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getX(n); // 64-bit variant
			}
			imm = (IR & 0x003FFC00) >> 10;
			B = imm;
			
			// registrador destino
			d = (IR & 0x0000001F);
			if (d == 31) {
				Rd = &SP;
			} else {
				Rd = &(R[d]);
			}
			
			// atribuir ALUctrl
			ALUctrl = ALUctrlFlag::SUB;
			
			// atribuir MEMctrl
			MEMctrl = MEMctrlFlag::MEM_NONE;
			
			// atribuir WBctrl
			WBctrl = WBctrlFlag::RegWrite;
			
			// atribuir MemtoReg
			MemtoReg = false;
			
			return 0;
		default:
			// instrução não implementada
			return 1;
	}
	
	// instrução não implementada
	return 1;
}

/**
 * ATIVIDADE FUTURA: Decodifica instruções do grupo
 * 		101x Branches, Exception Generating and System instructions
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::decodeBranches() {

	switch (IR & 0xFC000000){

		case 0x14000000:
		unsigned int imm26 = IR & 0x03FFFFFF;
		A = PC; 
		B = (((int32_t)imm26) << 6) >> 4;
		//PC = PC + B;

			// atribuir ALUctrl
			ALUctrl = ALUctrlFlag::ADD;
			
			// atribuir MEMctrl
			MEMctrl = MEMctrlFlag::WRITE32;
			
			// atribuir WBctrl
			WBctrl = WBctrlFlag::RegWrite;
			
			// atribuir MemtoReg
			MemtoReg = false;
		
		return 0;
	}
	// instrução não implementada
	return 1;
}

/**
 * ATIVIDADE FUTURA: Decodifica instruções do grupo
 * 		x1x0 Loads and Stores on page C4-246
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::decodeLoadStore() {
	unsigned int n, d, t;
	int imm;

	switch (IR & 0xFFC00000) {
	
	case 0xB9800000://LDRSW C6.2.131 Immediate (Unsigned offset) 913
		// como � escrita em 64 bits, n�o h� problema em decodificar

		n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getX(n); // 64-bit variant
			}

		B = (IR & 0x003ffc00) >> 8; // immediate
		//t = (IR & 0x0000001F;)
		Rd = &(R[IR & 0x0000001F]);  

		ALUctrl = ALUctrlFlag::ADD;
				
				// atribuir MEMctrl
				MEMctrl = MEMctrlFlag::READ64;
				
				// atribuir WBctrl
				WBctrl = WBctrlFlag::RegWrite;
				
				// atribuir MemtoReg
				MemtoReg = true; 
		
		return 0;
		break;

	case 0xB9000000://STR C6.2.257 Unsigned offset 1135
			// ler A e B
		n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getX(n); // 64-bit variant
			}

		B = ((IR & 0x003FFC00) >> 10) << 2; //offset = imm12 << scale. scale == size

		d = (IR & 0x0000001F);
			if (d == 31) {
				Rd = &SP;
			} else {
				Rd = &(R[d]);
			}

		// atribuir ALUctrl
				ALUctrl = ALUctrlFlag::ADD;
				
				// atribuir MEMctrl
				MEMctrl = MEMctrlFlag::WRITE32;
				
				// atribuir WBctrl
				WBctrl = WBctrlFlag::WB_NONE;	
				
				// atribuir MemtoReg
				MemtoReg = false;

		return 0;
		break;

		//default:
			// instrução não implementada
			//return 1;

		case 0xB9400000://LDR C6.2.119 Immediate (Unsigned offset) 886 
			//32 bits
		n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getX(n); // 64-bit variant
			}

			B = ((IR & 0x003FFC00) >> 10) << 2;

		d = (IR & 0x0000001F);
			if (d == 31) {
				Rd = &SP;
			} else {
				Rd = &(R[d]);
			}

		// atribuir ALUctrl
				ALUctrl = ALUctrlFlag::ADD;
				
				// atribuir MEMctrl
				MEMctrl = MEMctrlFlag::READ32;
				
				// atribuir WBctrl
				WBctrl = WBctrlFlag::RegWrite;
				
				// atribuir MemtoReg
				MemtoReg = true;

		return 0;
		break;
	}


	switch (IR & 0xFFE0FC00) {
		//1111 1111 1110 0000 1111 1100 0000 0000
	case 0xB8607800://LDR (Register) C6.2.121 891

			// ler A e B
		n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getX(n); // 64-bit variant
			}


		n = (IR & 0x001F0000) >> 16;
		if (n == 31) {
			B = SP << 2;
		}
		else {
			B = getX(n) << 2;// como eu considero no and as "variaveis" como 1, so vai entrar nesse case se for: size 10, option 011 e s 1
		}

		d = IR & 0x0000001F;
		Rd = &(R[d]);

		// atribuir ALUctrl
				ALUctrl = ALUctrlFlag::ADD;
				
				// atribuir MEMctrl
				MEMctrl = MEMctrlFlag::READ32;
				
				// atribuir WBctrl
				WBctrl = WBctrlFlag::RegWrite;
				
				// atribuir MemtoReg
				MemtoReg = true;

		return 0;
		break;
	}
	
	// instrução não implementada
	return 1;
}

/**
 * Decodifica instruções do grupo
 * 		x101 Data Processing -- Register on page C4-278
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::decodeDataProcReg() {
	// TODO
	//		acrescentar um switch no estilo do switch de decodeDataProcImm,
	//		e implementar APENAS PARA A INSTRUÇÃO A SEGUIR:
	//				'add w1, w1, w0'
	//		que aparece na linha 40 de isummation.S e no endereço 0x74
	//		de txt_isummation.o.txt.
	
	unsigned int n, m, shift, imm6, d;

	switch (IR & 0x7F200000)
	{
		case 0x0B000000:
			
			// IR & 80000000 para sf 64 bits

			// ler A e B
			//Só pra registrador W

			n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				A = SP;
			} else {
				A = getW(n); // 32-bit variant, sem implementação para 64-bits ainda
			}

			m = (IR & 0x001F0000) >> 16;
			B = getW(m);

			shift = (IR & 0x00C00000) >> 22;
			imm6  = (IR & 0x0000FC00) >> 10;
			 
			switch (shift)
			{
			case 0:
				B = B << imm6;
				break;
			case 1:
				B = ((unsigned long)B) >> imm6;
				break;
			case 2:
				B = ((signed long)B) >> imm6; // VARIAVEL FOI CONVERTIDA 'CASTING'
			default:
				break;
			}
			
			// ATIVIDADE FUTURA: implementar informações para os estágios
			// MEM e WB.

			// registrador destino
			d = (IR & 0x0000001F);
			if (d == 31) {
				Rd = &SP;
			} else {
				Rd = &(R[d]);
			}
			
			// atribuir ALUctrl
			ALUctrl = ALUctrlFlag::ADD;
			
			// atribuir MEMctrl
			MEMctrl = MEMctrlFlag::MEM_NONE;
			
			// atribuir WBctrl
			WBctrl = WBctrlFlag::RegWrite;
			
			// atribuir MemtoReg
			MemtoReg = false;

			return 0;
		default:
			// instrução não implementada
			return 1;
	}
	// instrução não implementada
	return 1;
}

/**
 * ATIVIDADE FUTURA: Decodifica instruções do grupo
 * 		x111 Data Processing -- Scalar Floating-Point and Advanced SIMD
 * 				on page C4-288
 *
 * Retorna 0: se executou corretamente e
 *		   1: se a instrução não estiver implementada.
 */
int BasicCPU::decodeDataProcFloat() {
	// instrução não implementada
	return 1;
}


/**
 * Execução lógico aritmética inteira.
 * 
 * Executa a operação lógico aritmética inteira com base nos valores
 * dos registradores auxiliares A, B e ALUctrl, e coloca o resultado
 * no registrador auxiliar ALUout.
 *
 * Retorna 0: se executou corretamente e
 *		   1: se o controle presente em ALUctrl não estiver implementado.
 */
int BasicCPU::EXI()
{
	// TODO
	//		Acrescente os cases no switch já iniciado, para acrescentar a
	//		execução APENAS PARA A INSTRUÇÃO A SEGUIR:
	//				'add w1, w1, w0'
	//		que aparece na linha 40 de isummation.S e no endereço 0x74
	//		de txt_isummation.o.txt.
	//
	// 		Verifique que ALUctrlFlag já tem declarado o tipo de operação
	//		executada por 'add w1, w1, w0'.
	switch (ALUctrl)
	{
		case ALUctrlFlag::SUB:
			ALUout = A - B;
			// ATIVIDADE FUTURA: setar flags NCZF
			return 0;
		case ALUctrlFlag::ADD:
			ALUout = A + B;
			return 0;
		default:
			// Controle não implementado
			return 1;
	}
	
	// Controle não implementado
	return 1;
};

		
/**
 * Execução lógico aritmética em ponto flutuante.
 * 
 * Executa a operação lógico aritmética em ponto flutuant com base
 * nos valores dos registradores auxiliares AF, BF e ALUctrl, e coloca o
 * resultado no registrador auxiliar ALUoutF.
 *
 * Retorna 0: se executou corretamente e
 *		   1: se o controle presente em ALUctrl não estiver implementado.
 */
int BasicCPU::EXF()
{
	// não implementado
	return 1;
}

/**
 * Acesso a dados na memória.
 * 
 * Retorna 0: se executou corretamente e
 *		   1: se o controle presente nos registradores auxiliares não
 * 				estiver implementado.
 */
int BasicCPU::MEM()
{
	// TODO
	// Implementar o switch (MEMctrl) case MEMctrlFlag::XXX com as
	// chamadas aos métodos corretos que implementam cada caso de
	// acesso à memória de dados.

	    switch (MEMctrl) {

    case MEMctrlFlag::READ32:
        MDR = memory->readData32(ALUout);
        return 0;
    case MEMctrlFlag::WRITE32:
        memory->writeData32(ALUout,*Rd);
        return 0;
    case MEMctrlFlag::READ64:
        MDR = memory->readData64(ALUout);
        return 0;
    case MEMctrlFlag::WRITE64:
        memory->writeData64(ALUout,*Rd);
        return 0;
    default:
        return 0;
}
    // não implementado
    return 1;
}


/**
 * Write-back. Escreve resultado da operação no registrador destino.
 * 
 * Retorna 0: se executou corretamente e
 *		   1: se o controle presente nos registradores auxiliares não
 * 				estiver implementado.
 */
int BasicCPU::WB()
{
	// TODO
	// Implementar o switch (WBctrl) case WBctrlFlag::XXX com as
	// atribuições corretas do registrador destino, quando houver, ou
	// return 0 no caso WBctrlFlag::WB_NONE.
	switch (WBctrl) {
        case WBctrlFlag::WB_NONE:
            return 0;
        case WBctrlFlag::RegWrite:
            if (MemtoReg) {
                *Rd = MDR;
            } else {
                *Rd = ALUout;
            }
            return 0;
        default:
            // não implementado
            return 1;
    }
}


/**
 * Métodos de acesso ao banco de registradores
 */

/**
 * Lê registrador inteiro de 32 bits.
 */
int BasicCPU::getW(int n) {
	long wn = 0x00000000FFFFFFFF & R[n];
	return ((int) wn);
}

/**
 * Escreve registrador inteiro de 32 bits.
 */
void BasicCPU::setW(int n, int value) {
	R[n] = (long)value;
}

/**
 * Lê registrador inteiro de 64 bits.
 */
int BasicCPU::getX(int n) {
	return R[n];
}

/**
 * Escreve registrador inteiro de 32 bits.
 */
void BasicCPU::setX(int n, long value) {
	R[n] = value;
}
