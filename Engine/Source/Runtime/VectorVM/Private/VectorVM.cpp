// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	VectorVM.cpp: Implementation of the vector virtual machine.
==============================================================================*/

#include "VectorVMPrivate.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, VectorVM);

DEFINE_LOG_CATEGORY_STATIC(LogVectorVM, All, All);

#define SRCOP_RRR 0x00
#define SRCOP_RRC 0x01
#define SRCOP_RCR 0x02
#define SRCOP_RCC 0x03
#define SRCOP_CRC 0x05
#define SRCOP_CCR 0x06
#define SRCOP_CCC 0x07

/**
 * Context information passed around during VM execution.
 */
struct FVectorVMContext
{
	/** Pointer to the next element in the byte code. */
	uint8 const* RESTRICT Code;
	/** Pointer to the table of vector register arrays. */
	VectorRegister* RESTRICT * RESTRICT RegisterTable;
	/** Pointer to the constant table. */
	FVector4 const* RESTRICT ConstantTable;
	/** The number of vectors to process. */
	int32 NumVectors;

	/** Initialization constructor. */
	FVectorVMContext(
		uint8 const* InCode,
		VectorRegister** InRegisterTable,
		FVector4 const* InConstantTable,
		int32 InNumVectors
		)
		: Code(InCode)
		, RegisterTable(InRegisterTable)
		, ConstantTable(InConstantTable)
		, NumVectors(InNumVectors)
	{
	}
};

/** Decode the next operation contained in the bytecode. */
static FORCEINLINE VectorVM::EOp::Type DecodeOp(FVectorVMContext& Context)
{
	return static_cast<VectorVM::EOp::Type>(*Context.Code++);
}

/** Decode a register from the bytecode. */
static FORCEINLINE VectorRegister* DecodeRegister(FVectorVMContext& Context)
{
	return Context.RegisterTable[*Context.Code++];
}

/** Decode a constant from the bytecode. */
static FORCEINLINE VectorRegister DecodeConstant(FVectorVMContext& Context)
{
	const FVector4* vec = &Context.ConstantTable[*Context.Code++];
	return VectorLoad(vec);
}

static FORCEINLINE uint8 DecodeSrcOperandTypes(FVectorVMContext& Context)
{
	return *Context.Code++;
}

/** Base class for vector kernels with one dest and one src operand. */
template <typename Kernel>
struct TUnaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);
		int32 NumVectors = Context.NumVectors;

		if (SrcOpTypes == SRCOP_RRR)
		{
			VectorRegister* Src0 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++,*Src0++);
			}
		}
		else if (SrcOpTypes == SRCOP_RRC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0);
			}
		}

	}
};


/** Base class for vector kernels with one dest and two src operands. */
template <typename Kernel>
struct TBinaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);

		int32 NumVectors = Context.NumVectors;
		if (SrcOpTypes == SRCOP_RRR)
		{
			VectorRegister* Src0 = DecodeRegister(Context);
			VectorRegister* Src1 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, *Src0++, *Src1++);
			}
		}
		else if (SrcOpTypes == SRCOP_RRC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			VectorRegister *Src1 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0, *Src1++);
			}
		}
		else if (SrcOpTypes == SRCOP_RCR)
		{
			VectorRegister *Src0 = DecodeRegister(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, *Src0++, Src1);
			}
		}
		else if (SrcOpTypes == SRCOP_RCC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0, Src1);
			}
		}
	}
};

/** Base class for vector kernels with one dest and three src operands. */
template <typename Kernel>
struct TTrinaryVectorKernel
{
	static void Exec(FVectorVMContext& Context)
	{
		VectorRegister* RESTRICT Dst = DecodeRegister(Context);
		uint32 SrcOpTypes = DecodeSrcOperandTypes(Context);

		int32 NumVectors = Context.NumVectors;
		if (SrcOpTypes == SRCOP_RRR)
		{
			VectorRegister* Src0 = DecodeRegister(Context);
			VectorRegister* Src1 = DecodeRegister(Context);
			VectorRegister* Src2 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, *Src0++, *Src1++, *Src2++);
			}
		}
		else if (SrcOpTypes == SRCOP_RRC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			VectorRegister* Src1 = DecodeRegister(Context);
			VectorRegister* Src2 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0, *Src1++, *Src2++);
			}
		}
		else if (SrcOpTypes == SRCOP_CCC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			VectorRegister Src2 = DecodeConstant(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0, Src1, Src2);
			}
		}
		else if (SrcOpTypes == SRCOP_CCR)
		{
			VectorRegister *Src0 = DecodeRegister(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			VectorRegister Src2 = DecodeConstant(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, *Src0++, Src1, Src2);
			}
		}
		else if (SrcOpTypes == SRCOP_RCC)
		{
			VectorRegister Src0 = DecodeConstant(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			VectorRegister *Src2 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, Src0, Src1, *Src2++);
			}
		}
		else if (SrcOpTypes == SRCOP_RCR)
		{
			VectorRegister *Src0 = DecodeRegister(Context);
			VectorRegister Src1 = DecodeConstant(Context);
			VectorRegister *Src2 = DecodeRegister(Context);
			for (int32 i = 0; i < NumVectors; ++i)
			{
				Kernel::DoKernel(Dst++, *Src0++, Src1, *Src2++);
			}
		}
	}
};

/*------------------------------------------------------------------------------
	Implementation of all kernel operations.
------------------------------------------------------------------------------*/

struct FVectorKernelAdd : public TBinaryVectorKernel<FVectorKernelAdd>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorAdd(Src0, Src1);
	}
};

struct FVectorKernelSub : public TBinaryVectorKernel<FVectorKernelSub>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorSubtract(Src0, Src1);
	}
};

struct FVectorKernelMul : public TBinaryVectorKernel<FVectorKernelMul>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMultiply(Src0, Src1);
	}
};

struct FVectorKernelMad : public TTrinaryVectorKernel<FVectorKernelMad>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		*Dst = VectorMultiplyAdd(Src0, Src1, Src2);
	}
};

struct FVectorKernelLerp : public TTrinaryVectorKernel<FVectorKernelLerp>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		const VectorRegister One = MakeVectorRegister(1.0f, 1.0f, 1.0f, 1.0f);
		const VectorRegister OneMinusAlpha = VectorSubtract(One, Src2);
		const VectorRegister Tmp = VectorMultiply(Src0, OneMinusAlpha);
		*Dst = VectorMultiplyAdd(Src1, Src2, Tmp);
	}
};

struct FVectorKernelRcp : public TUnaryVectorKernel<FVectorKernelRcp>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorReciprocal(Src0);
	}
};

struct FVectorKernelRsq : public TUnaryVectorKernel<FVectorKernelRsq>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorReciprocalSqrt(Src0);
	}
};

struct FVectorKernelSqrt : public TUnaryVectorKernel<FVectorKernelSqrt>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		// TODO: Need a SIMD sqrt!
		float* RESTRICT FloatDst = reinterpret_cast<float* RESTRICT>(Dst);
		float const* FloatSrc0 = reinterpret_cast<float const*>(&Src0);
		FloatDst[0] = FMath::Sqrt(FloatSrc0[0]);
		FloatDst[1] = FMath::Sqrt(FloatSrc0[1]);
		FloatDst[2] = FMath::Sqrt(FloatSrc0[2]);
		FloatDst[3] = FMath::Sqrt(FloatSrc0[3]);
	}
};

struct FVectorKernelNeg : public TUnaryVectorKernel<FVectorKernelNeg>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorNegate(Src0);
	}
};

struct FVectorKernelAbs : public TUnaryVectorKernel<FVectorKernelAbs>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0)
	{
		*Dst = VectorAbs(Src0);
	}
};

struct FVectorKernelClamp : public TTrinaryVectorKernel<FVectorKernelClamp>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1,VectorRegister Src2)
	{
		const VectorRegister Tmp = VectorMax(Src0, Src1);
		*Dst = VectorMin(Tmp, Src2);
	}
};


struct FVectorKernelSin : public TUnaryVectorKernel<FVectorKernelSin>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		float v = VectorGetComponent(Src0, 0);
		float sn = FMath::Sin(v*3.14f);		// [0;1] takes us through half a period
		*Dst = MakeVectorRegister(sn, sn, sn, sn);
	}
};

struct FVectorKernelDot : public TBinaryVectorKernel<FVectorKernelDot>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorDot4(Src0, Src1);
	}
};


struct FVectorKernelLength : public TUnaryVectorKernel<FVectorKernelLength>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		VectorRegister Temp = VectorDot4(Src0, Src0);
		float const* FloatSrc = reinterpret_cast<float const*>(&Temp);
		float SDot = FMath::Sqrt(FloatSrc[0]);
		*Dst = MakeVectorRegister(SDot, SDot, SDot, SDot);
	}
};


struct FVectorKernelCross : public TBinaryVectorKernel<FVectorKernelCross>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0, VectorRegister Src1)
	{
		*Dst = VectorCross(Src0, Src1);
	}
};

struct FVectorKernelNormalize : public TUnaryVectorKernel<FVectorKernelNormalize>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		*Dst = VectorNormalize(Src0);
	}
};

struct FVectorKernelRandom : public TUnaryVectorKernel<FVectorKernelRandom>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst, VectorRegister Src0)
	{
		const float rm = RAND_MAX;
		VectorRegister Result = MakeVectorRegister(static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm,
			static_cast<float>(FMath::Rand()) / rm);
		*Dst = VectorMultiply(Result, Src0);
		
	}
};



struct FVectorKernelMin : public TBinaryVectorKernel<FVectorKernelMin>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMin(Src0, Src1);
	}
};

struct FVectorKernelMax : public TBinaryVectorKernel<FVectorKernelMax>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorMax(Src0, Src1);
	}
};

struct FVectorKernelPow : public TBinaryVectorKernel<FVectorKernelPow>
{
	static void FORCEINLINE DoKernel(VectorRegister* RESTRICT Dst,VectorRegister Src0,VectorRegister Src1)
	{
		*Dst = VectorPow(Src0, Src1);
	}
};




void VectorVM::Exec(
	uint8 const* Code,
	VectorRegister** InputRegisters,
	int32 NumInputRegisters,
	VectorRegister** OutputRegisters,
	int32 NumOutputRegisters,
	FVector4 const* ConstantTable,
	int32 NumVectors
	)
{
	VectorRegister TempRegisters[NumTempRegisters][VectorsPerChunk];
	VectorRegister* RegisterTable[MaxRegisters] = {0};

	// Map temporary registers.
	for (int32 i = 0; i < NumTempRegisters; ++i)
	{
		RegisterTable[i] = TempRegisters[i];
	}

	// Process one chunk at a time.
	int32 NumChunks = (NumVectors + VectorsPerChunk - 1) / VectorsPerChunk;
	for (int32 ChunkIndex = 0; ChunkIndex < NumChunks; ++ChunkIndex)
	{
		// Map input and output registers.
		for (int32 i = 0; i < NumInputRegisters; ++i)
		{
			RegisterTable[NumTempRegisters + i] = InputRegisters[i] + ChunkIndex * VectorsPerChunk;
		}
		for (int32 i = 0; i < NumOutputRegisters; ++i)
		{
			RegisterTable[NumTempRegisters + MaxInputRegisters + i] = OutputRegisters[i] + ChunkIndex * VectorsPerChunk;
		}

		// Setup execution context.
		int32 VectorsThisChunk = FMath::Min<int32>(NumVectors, VectorsPerChunk);
		FVectorVMContext Context(Code, RegisterTable, ConstantTable, VectorsThisChunk);
		EOp::Type Op = EOp::done;

		// Execute VM on all vectors in this chunk.
		do 
		{
			Op = DecodeOp(Context);
			switch (Op)
			{
			// Dispatch kernel ops.
			case EOp::add: FVectorKernelAdd::Exec(Context); break;
			case EOp::sub: FVectorKernelSub::Exec(Context); break;
			case EOp::mul: FVectorKernelMul::Exec(Context); break;
			case EOp::mad: FVectorKernelMad::Exec(Context); break;
			case EOp::lerp: FVectorKernelLerp::Exec(Context); break;
			case EOp::rcp: FVectorKernelRcp::Exec(Context); break;
			case EOp::rsq: FVectorKernelRsq::Exec(Context); break;
			case EOp::sqrt: FVectorKernelSqrt::Exec(Context); break;
			case EOp::neg: FVectorKernelNeg::Exec(Context); break;
			case EOp::abs: FVectorKernelAbs::Exec(Context); break;
			case EOp::clamp: FVectorKernelClamp::Exec(Context); break;
			case EOp::min: FVectorKernelMin::Exec(Context); break;
			case EOp::max: FVectorKernelMax::Exec(Context); break;
			case EOp::pow: FVectorKernelPow::Exec(Context); break;
			case EOp::sin: FVectorKernelSin::Exec(Context); break;
			case EOp::dot: FVectorKernelDot::Exec(Context); break;
			case EOp::length: FVectorKernelLength::Exec(Context); break;
			case EOp::cross: FVectorKernelCross::Exec(Context); break;
			case EOp::normalize: FVectorKernelNormalize::Exec(Context); break;
			case EOp::random: FVectorKernelRandom::Exec(Context); break;

			// Execution always terminates with a "done" opcode.
			case EOp::done:
				break;

			// Opcode not recognized / implemented.
			default:
				UE_LOG(LogVectorVM, Fatal, TEXT("Unknown op code 0x%02x"), (uint32)Op);
				break;
			}
		} while (Op != EOp::done);

		NumVectors -= VectorsPerChunk;
	}
}

namespace VectorVM
{
	static FVectorVMOpInfo GOpInfo[] =
	{
		FVectorVMOpInfo(EOp::done, EOpFlags::None, EOpSrc::Invalid, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("done")),

		FVectorVMOpInfo(EOp::add, EOpFlags::Implemented | EOpFlags::Commutative, EOpSrc::Register, EOpSrc::Register, EOpSrc::Invalid, TEXT("Add")),
		FVectorVMOpInfo(EOp::add, EOpFlags::None, EOpSrc::Register, EOpSrc::Const, EOpSrc::Invalid, TEXT("addi")),

		FVectorVMOpInfo(EOp::sub, EOpFlags::Implemented, EOpSrc::Register, EOpSrc::Register, EOpSrc::Invalid, TEXT("Sub")),
		FVectorVMOpInfo(EOp::sub, EOpFlags::None, EOpSrc::Register, EOpSrc::Const, EOpSrc::Invalid, TEXT("subi")),

		FVectorVMOpInfo(EOp::mul, EOpFlags::Implemented | EOpFlags::Commutative, EOpSrc::Register, EOpSrc::Register, EOpSrc::Invalid, TEXT("Multiply")),
		FVectorVMOpInfo(EOp::mul,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Invalid,TEXT("muli")),

		FVectorVMOpInfo(EOp::mad,EOpFlags::Implemented|EOpFlags::Commutative,EOpSrc::Register,EOpSrc::Register,EOpSrc::Register,TEXT("Multply-Add")),
		FVectorVMOpInfo(EOp::mad,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Const,TEXT("madrri")),
		FVectorVMOpInfo(EOp::mad,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Register,TEXT("madrir")),
		FVectorVMOpInfo(EOp::mad,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Const,TEXT("madrii")),
		FVectorVMOpInfo(EOp::mad,EOpFlags::None,EOpSrc::Const,EOpSrc::Const,EOpSrc::Register,TEXT("madiir")),
		FVectorVMOpInfo(EOp::mad,EOpFlags::None,EOpSrc::Const,EOpSrc::Const,EOpSrc::Const,TEXT("madiii")),

		FVectorVMOpInfo(EOp::lerp,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Register,EOpSrc::Register,TEXT("Lerp")),
		FVectorVMOpInfo(EOp::lerp,EOpFlags::None,EOpSrc::Const,EOpSrc::Register,EOpSrc::Register,TEXT("lerpirr")),
		FVectorVMOpInfo(EOp::lerp,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Register,TEXT("lerprir")), 
		FVectorVMOpInfo(EOp::lerp,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Const,TEXT("lerprri")),
		FVectorVMOpInfo(EOp::lerp,EOpFlags::None,EOpSrc::Const,EOpSrc::Const,EOpSrc::Register,TEXT("lerpiir")),

		FVectorVMOpInfo(EOp::rcp,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Reciprocal")),
		FVectorVMOpInfo(EOp::rsq,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Reciprocal Sqrt")),
		FVectorVMOpInfo(EOp::sqrt,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Sqrt")),
		FVectorVMOpInfo(EOp::neg,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Negate")),
		FVectorVMOpInfo(EOp::abs,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Absolute")),
		FVectorVMOpInfo(EOp::exp,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Exp")),
		FVectorVMOpInfo(EOp::exp2,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Exp2")),
		FVectorVMOpInfo(EOp::log,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Log")),
		FVectorVMOpInfo(EOp::log2,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Log base 2")),
		FVectorVMOpInfo(EOp::sin,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Sin")),
		FVectorVMOpInfo(EOp::sin, EOpFlags::None, EOpSrc::Const, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("sini")),
		FVectorVMOpInfo(EOp::cos, EOpFlags::None, EOpSrc::Register, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("Cos")),
		FVectorVMOpInfo(EOp::tan,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Tan")),
		FVectorVMOpInfo(EOp::asin,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Arcsin")),
		FVectorVMOpInfo(EOp::acos,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Arccos")),
		FVectorVMOpInfo(EOp::atan,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Arctan")),
		FVectorVMOpInfo(EOp::atan2,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Arctan2")),
		FVectorVMOpInfo(EOp::ceil,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Round up")),
		FVectorVMOpInfo(EOp::floor,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Round down")),
		FVectorVMOpInfo(EOp::fmod,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Modulo")),
		FVectorVMOpInfo(EOp::frac,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Fractional")),
		FVectorVMOpInfo(EOp::trunc,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Trunc")),

		FVectorVMOpInfo(EOp::clamp,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Register,EOpSrc::Register,TEXT("Clamp")),
		FVectorVMOpInfo(EOp::clamp,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Register,TEXT("clampir")),
		FVectorVMOpInfo(EOp::clamp,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Const,TEXT("clampri")),
		FVectorVMOpInfo(EOp::clamp,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Const,TEXT("clampii")),

		FVectorVMOpInfo(EOp::min,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Min")),
		FVectorVMOpInfo(EOp::min,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Invalid,TEXT("mini")),

		FVectorVMOpInfo(EOp::max,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Max")),
		FVectorVMOpInfo(EOp::max,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Invalid,TEXT("maxi")),

		FVectorVMOpInfo(EOp::pow,EOpFlags::Implemented,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Pow")),
		FVectorVMOpInfo(EOp::pow,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Invalid,TEXT("powi")),

		FVectorVMOpInfo(EOp::add,EOpFlags::None,EOpSrc::Register,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("Sign")),

		FVectorVMOpInfo(EOp::step,EOpFlags::None,EOpSrc::Register,EOpSrc::Register,EOpSrc::Invalid,TEXT("Step")),
		FVectorVMOpInfo(EOp::step,EOpFlags::None,EOpSrc::Register,EOpSrc::Const,EOpSrc::Invalid,TEXT("stepi")),

		FVectorVMOpInfo(EOp::add,EOpFlags::None,EOpSrc::Invalid,EOpSrc::Invalid,EOpSrc::Invalid,TEXT("tex1d")),

		FVectorVMOpInfo(EOp::dot, EOpFlags::Implemented, EOpSrc::Register, EOpSrc::Register, EOpSrc::Invalid, TEXT("Dot Product")),
		FVectorVMOpInfo(EOp::cross, EOpFlags::Implemented|EOpFlags::Commutative, EOpSrc::Register, EOpSrc::Register, EOpSrc::Invalid, TEXT("Cross Product")),
		FVectorVMOpInfo(EOp::cross, EOpFlags::None, EOpSrc::Register, EOpSrc::Const, EOpSrc::Invalid, TEXT("Cross Product with const")),

		FVectorVMOpInfo(EOp::normalize, EOpFlags::Implemented, EOpSrc::Register, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("Normalize")),
		FVectorVMOpInfo(EOp::random, EOpFlags::Implemented, EOpSrc::Const, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("Random")),

		FVectorVMOpInfo(EOp::length, EOpFlags::Implemented, EOpSrc::Register, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("Vector Length")),
		FVectorVMOpInfo(EOp::length, EOpFlags::None, EOpSrc::Const, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("Vector Length (const)")),
	
		FVectorVMOpInfo(EOp::add, EOpFlags::None, EOpSrc::Invalid, EOpSrc::Invalid, EOpSrc::Invalid, TEXT("invalid"))
	};
} // namespace VectorVM

VectorVM::FVectorVMOpInfo const& VectorVM::GetOpCodeInfo(uint8 OpCodeIndex)
{
	return GOpInfo[FMath::Clamp<int32>(OpCodeIndex, 0, EOp::NumOpcodes)];
}


uint8 VectorVM::GetNumOpCodes()
{
	return EOp::NumOpcodes;
}

/*------------------------------------------------------------------------------
	Automation test for the VM.
------------------------------------------------------------------------------*/

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVectorVMTest, "Core.Math.Vector VM", EAutomationTestFlags::ATF_SmokeTest)

bool FVectorVMTest::RunTest(const FString& Parameters)
{
	uint8 TestCode[] =
	{
		VectorVM::EOp::mul,		0x00, SRCOP_RRR, 0x08, 0x08,       // mul r0, r8, r8
		VectorVM::EOp::mad,		0x01, SRCOP_RRR, 0x09, 0x09, 0x00, // mad r1, r9, r9, r0
		VectorVM::EOp::mad,		0x00, SRCOP_RRR, 0x0a, 0x0a, 0x01, // mad r0, r10, r10, r1
		VectorVM::EOp::add,		0x01, SRCOP_RRC, 0x00, 0x01,       // addi r1, r0, c1
		VectorVM::EOp::neg,		0x00, SRCOP_RRR, 0x01,             // neg r0, r1
		VectorVM::EOp::clamp,	0x28, SRCOP_CCR, 0x00, 0x02, 0x03, // clampii r40, r0, c2, c3
		0x00 // terminator
	};

	VectorRegister TestRegisters[4][VectorVM::VectorsPerChunk];
	VectorRegister* InputRegisters[3] = { TestRegisters[0], TestRegisters[1], TestRegisters[2] };
	VectorRegister* OutputRegisters[1] = { TestRegisters[3] };

	VectorRegister Inputs[3][VectorVM::VectorsPerChunk];
	for (int32 i = 0; i < VectorVM::ChunkSize; i++)
	{
		reinterpret_cast<float*>(&Inputs[0])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(&Inputs[1])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(&Inputs[2])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[0])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[1])[i] = static_cast<float>(i);
		reinterpret_cast<float*>(InputRegisters[2])[i] = static_cast<float>(i);
	}

	FVector4 ConstantTable[VectorVM::MaxConstants];
	ConstantTable[0] = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
	ConstantTable[1] = FVector4(5.0f, 5.0f, 5.0f, 5.0f);
	ConstantTable[2] = FVector4(-20.0f, -20.0f, -20.0f, -20.0f);
	ConstantTable[3] = FVector4(20.0f, 20.0f, 20.0f, 20.0f);

	VectorVM::Exec(
		TestCode,
		InputRegisters, 3,
		OutputRegisters, 1,
		ConstantTable,
		VectorVM::VectorsPerChunk
		);

	for (int32 i = 0; i < VectorVM::ChunkSize; i++)
	{
		float Ins[3];

		// Verify that the input registers were not overwritten.
		for (int32 InputIndex = 0; InputIndex < 3; ++InputIndex)
		{
			float In = Ins[InputIndex] = reinterpret_cast<float*>(&Inputs[InputIndex])[i];
			float R = reinterpret_cast<float*>(InputRegisters[InputIndex])[i];
			if (In != R)
			{
				UE_LOG(LogVectorVM,Error,TEXT("Input register %d vector %d element %d overwritten. Has %f expected %f"),
					InputIndex,
					i / VectorVM::ElementsPerVector,
					i % VectorVM::ElementsPerVector,
					R,
					In
					);
				return false;
			}
		}

		// Verify that outputs match what we expect.
		float Out = reinterpret_cast<float*>(OutputRegisters[0])[i];
		float Expected = FMath::Clamp<float>(-(Ins[0] * Ins[0] + Ins[1] * Ins[1] + Ins[2] * Ins[2] + 5.0f), -20.0f, 20.0f);
		if (Out != Expected)
		{
			UE_LOG(LogVectorVM,Error,TEXT("Output register %d vector %d element %d is wrong. Has %f expected %f"),
				0,
				i / VectorVM::ElementsPerVector,
				i % VectorVM::ElementsPerVector,
				Out,
				Expected
				);
			return false;
		}
	}

	return true;
}
