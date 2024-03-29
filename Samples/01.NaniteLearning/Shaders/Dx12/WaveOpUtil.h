// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

uint CountBits( uint2 Bits )
{
	return countbits( Bits.x ) + countbits( Bits.y );
}

#ifndef WaveReadLaneLast
uint WaveGetActiveLaneIndexLast()
{
	uint2 ActiveMask = WaveActiveBallot(true).xy;
	return firstbithigh(ActiveMask.y ? ActiveMask.y : ActiveMask.x) + (ActiveMask.y ? 32 : 0);
}

#define WaveReadLaneLast(x) WaveReadLaneAt( x, WaveGetActiveLaneIndexLast() )
#endif

/*
===============================================================================
Below are a series of WaveInterlockedAdd (and WaveInterlockedMin/Max) macros used for aggregated atomics.

There is a varying and scalar version of each type, scalar being faster. Scalar refers to Value. I can't check if the scalar version
is actually called with a scalar since HLSL doesn't allow that so be careful.

These are all forms of aggregated atomics which replace an extremely common pattern in compute when appending to a buffer. 

To avoid lots of global atomic traffic you typically follow this pattern:
* if( GroupIndex == 0 ) set groupshared counter to zero
* Group sync outside of flow control
* Accumulate locally using InterlockedAdd to groupshared
* Get the local offset amongst the group backGroup sync outside of flow control
* if( GroupIndex == 0 ) do a single global InterlockedAdd to the buffer
* Get the global offset bac
* Add local offset to global offset to derive the actual index of the element you appended
* Finally write the element value you wanted to append to the buffer at that index

This is a pain, forces any such operation into at least 4 code blocks, and generally makes a mess of things, especially when you
have many different counters in the same shader. It also adds syncs and LDS traffic which might slow things down.

All this is unnecessary as it can be done per wave instead. Some PC drivers automatically convert InterlockedAdd's to scalar addresses
to this for you. Console compilers definitely do not. The above macros do this for you. They can be used whether waveops are supported
or not and simply fallback to slower non-aggregated atomics when they aren't. Drivers may then do the optimization for you if they do that.
Using these macros which use wave ops is almost certainly faster. Using them is definitely cleaner and easier.

The InGroups versions implement another common pattern which is needing to set indirect args in increments of work groups, not threads.

Unfortunately I had to use macros as we don't have templates and I have no idea anyways how to pass a reference to an element of a RWBuffer
that's to be written to as a parameter on all platforms. Also unfortunate is that you can't overload a macro, hence the _ names.
===============================================================================
*/

#if COMPILER_SUPPORTS_WAVE_VOTE

#define WaveInterlockedAddScalar( Dest, Value ) \
{ \
	uint NumToAdd = WaveActiveCountBits( true ) * Value; \
	if( WaveIsFirstLane() ) \
		InterlockedAdd( Dest, NumToAdd ); \
}

#define WaveInterlockedAddScalar_( Dest, Value, OriginalValue ) \
{ \
	uint NumToAdd = WaveActiveCountBits( true ) * Value; \
	if( WaveIsFirstLane() ) \
		InterlockedAdd( Dest, NumToAdd, OriginalValue ); \
	OriginalValue = WaveReadLaneFirst( OriginalValue ) + WavePrefixCountBits( true ) * Value; \
}

#define WaveInterlockedAdd( Dest, Value ) \
{ \
	uint NumToAdd = WaveActiveSum( Value ); \
	if( WaveIsFirstLane() ) \
		InterlockedAdd( Dest, NumToAdd ); \
}

#define WaveInterlockedAdd_( Dest, Value, OriginalValue ) \
{ \
	uint LocalOffset = WavePrefixSum( Value ); \
	uint NumToAdd = WaveReadLaneLast( LocalOffset + Value ); \
	if( WaveIsFirstLane() ) \
		InterlockedAdd( Dest, NumToAdd, OriginalValue ); \
	OriginalValue = WaveReadLaneFirst( OriginalValue ) + LocalOffset; \
}

#define WaveInterlockedAddScalarInGroups( Dest, DestGroups, GroupsOf, Value, OriginalValue ) \
{ \
	uint NumToAdd = WaveActiveCountBits( true ) * Value; \
	if( WaveIsFirstLane() ) \
	{ \
		InterlockedAdd( Dest, NumToAdd, OriginalValue ); \
		InterlockedMax( DestGroups, ( OriginalValue + NumToAdd + GroupsOf - 1 ) / GroupsOf ); \
	} \
	OriginalValue = WaveReadLaneFirst( OriginalValue ) + WavePrefixCountBits( true ) * Value; \
}

#define WaveInterlockedAddInGroups( Dest, DestGroups, GroupsOf, Value, OriginalValue ) \
{ \
	uint LocalOffset = WavePrefixSum( Value ); \
	uint NumToAdd = WaveReadLaneLast( LocalOffset + Value ); \
	if( WaveIsFirstLane() ) \
	{ \
		InterlockedAdd( Dest, NumToAdd, OriginalValue ); \
		InterlockedMax( DestGroups, ( OriginalValue + NumToAdd + GroupsOf - 1 ) / GroupsOf ); \
	} \
	OriginalValue = WaveReadLaneFirst( OriginalValue ) + LocalOffset; \
}

#define WaveInterlockedMin( Dest, Value ) \
{ \
	uint MinValue = WaveActiveMin( Value ); \
	if( WaveIsFirstLane() ) \
		InterlockedMin( Dest, MinValue ); \
}

#define WaveInterlockedMax( Dest, Value ) \
{ \
	uint MaxValue = WaveActiveMax( Value ); \
	if( WaveIsFirstLane() ) \
		InterlockedMax( Dest, MaxValue ); \
}

#else

#define WaveInterlockedAddScalar( Dest, Value )					InterlockedAdd( Dest, Value )
#define WaveInterlockedAddScalar_( Dest, Value, OriginalValue )	InterlockedAdd( Dest, Value, OriginalValue )
#define WaveInterlockedAdd( Dest, Value )						InterlockedAdd( Dest, Value )
#define WaveInterlockedAdd_( Dest, Value, OriginalValue )		InterlockedAdd( Dest, Value, OriginalValue )

#define WaveInterlockedAddScalarInGroups( Dest, DestGroups, GroupsOf, Value, OriginalValue ) \
{ \
	InterlockedAdd( Dest, Value, OriginalValue ); \
	InterlockedMax( DestGroups, ( OriginalValue + Value + GroupsOf - 1 ) / GroupsOf ); \
}

#define WaveInterlockedAddInGroups( Dest, DestGroups, GroupsOf, Value, OriginalValue ) \
{ \
	InterlockedAdd( Dest, Value, OriginalValue ); \
	InterlockedMax( DestGroups, ( OriginalValue + Value + GroupsOf - 1 ) / GroupsOf ); \
}

#define WaveInterlockedMin( Dest, Value ) { InterlockedMin(Dest, Value); }
#define WaveInterlockedMax( Dest, Value ) { InterlockedMax(Dest, Value); }

#endif // PLATFORM_SUPPORTS_SM6_0_WAVE_OPERATIONS