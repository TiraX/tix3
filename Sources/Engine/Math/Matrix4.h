/*
	TiX Engine v3.0 Copyright (C) 2022~2025
	By ZhaoShuai tirax.cn@gmail.com
*/

#pragma once

namespace tix
{
	//all matrices types must have operator[]
	template<typename MT1, typename MT2, typename MT3>
	inline void RowMatrixProduct(MT1& out, const MT2& m1, const MT3& m2)
	{
		out[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
		out[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
		out[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
		out[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];

		out[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
		out[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
		out[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
		out[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];

		out[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
		out[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
		out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
		out[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];

		out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
		out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
		out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
		out[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
	}

	//all matrices types must have operator[]
	template<typename MT1, typename MT2, typename MT3>
	static inline void RowMatrixProduct34(MT1& out, const MT2& m1, const MT3& m2)
	{
		out[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2];
		out[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2];
		out[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2];
		out[3] = 0;

		out[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6];
		out[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6];
		out[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6];
		out[7] = 0;

		out[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10];
		out[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10];
		out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10];
		out[11] = 0;

		out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12];
		out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13];
		out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14];
		out[15] = 1;
	}



	//! 4x4 matrix. Mostly used as transformation matrix for 3d calculations.
	/** The matrix is a D3D style matrix, row major with translations in the 4th row. */
	template <class T>
	class FMatrix4
	{
	public:

		//! Constructor Flags
		enum eConstructor
		{
			EM4CONST_NOTHING = 0,
			EM4CONST_COPY,
			EM4CONST_IDENTITY,
			EM4CONST_TRANSPOSED,
			EM4CONST_INVERSE,
			EM4CONST_INVERSE_TRANSPOSED
		};

		FMatrix4(eConstructor constructor = EM4CONST_IDENTITY);

		FMatrix4(T _00, T _01, T _02, T _03,
			T _10, T _11, T _12, T _13,
			T _20, T _21, T _22, T _23,
			T _30, T _31, T _32, T _33);

		FMatrix4(const FMatrix4<T>& other, eConstructor constructor = EM4CONST_COPY);

		T& operator()(const int row, const int col)
		{
			return M[row * 4 + col];
		}

		const T& operator()(const int row, const int col) const
		{
			return M[row * 4 + col];
		}

		T& operator[](uint32 index)
		{
			return M[index];
		}

		const T& operator[](uint32 index) const
		{
			return M[index];
		}

		inline FMatrix4<T>& operator=(const FMatrix4<T>& other);
		inline FMatrix4<T>& operator=(const T& scalar);

		const T* pointer() const
		{
			return M;
		}
		T* pointer()
		{
			return M;
		}

		bool operator==(const FMatrix4<T>& other) const;
		bool operator!=(const FMatrix4<T>& other) const;

		FMatrix4<T> operator+(const FMatrix4<T>& other) const;
		FMatrix4<T>& operator+=(const FMatrix4<T>& other);
		FMatrix4<T> operator-(const FMatrix4<T>& other) const;
		FMatrix4<T>& operator-=(const FMatrix4<T>& other);

		inline FMatrix4<T>& Setbyproduct(const FMatrix4<T>& other_a, const FMatrix4<T>& other_b);
		FMatrix4<T>& Setbyproduct_nocheck(const FMatrix4<T>& other_a, const FMatrix4<T>& other_b);

		FMatrix4<T> operator*(const FMatrix4<T>& other) const;
		FMatrix4<T>& operator*=(const FMatrix4<T>& other);

		FMatrix4<T> Mult34(const FMatrix4<T>& m2) const;
		FMatrix4<T>& Mult34(const FMatrix4<T>& m2, FMatrix4<T>& out) const;

		FMatrix4<T> operator*(const T& scalar) const;
		FMatrix4<T>& operator*=(const T& scalar);

		inline FMatrix4<T>& MakeIdentity();

		FVec3<T> GetColumn(uint32 c) const;
		FMatrix4<T>& SetColumn(uint32 c, const FVec3<T>& v);

		FVec3<T> GetTranslation() const;
		FMatrix4<T>& SetTranslation(const FVec3<T>& translation);

		// Make a rotation matrix from Euler angles.
		inline FMatrix4<T>& SetRotationRadians(const FVec3<T>& rotation);
		FVec3<T> GetRotationRadians() const;

		FMatrix4<T>& SetScale(const FVec3<T>& scale);
		FMatrix4<T>& SetScale(const T scale)
		{
			return SetScale(FVec3<T>(scale, scale, scale));
		}

		// Apply scale to this matrix as if multiplication was on the left.
		FMatrix4<T>& PreScale(const FVec3<T>& scale);
		// Apply scale to this matrix as if multiplication was on the right.
		FMatrix4<T>& PostScale(const FVec3<T>& scale);

		FVec3<T> GetScale() const;

		//! Translate a vector by the inverse of the translation part of this matrix.
		void InverseTranslateVect(FVec3<T>& vect) const;

		//! Rotate a vector by the inverse of the rotation part of this matrix.
		void InverseRotateVect(FVec3<T>& vect) const;

		//! Rotate a vector by the rotation part of this matrix.
		void RotateVect(FVec3<T>& vect) const;

		//! An alternate transform vector method, writing into a second vector
		void RotateVect(FVec3<T>& out, const FVec3<T>& in) const;

		//! An alternate transform vector method, writing into an array of 3 floats
		void RotateVect(T* out, const FVec3<T>& in) const;

		//! Transforms the vector by this matrix
		void TransformVect(FVec3<T>& vect) const;

		//! Transforms input vector by this matrix and stores result in output vector
		void TransformVect(FVec3<T>& out, const FVec3<T>& in) const;
		void TransformVect(FVec3<T>& out, const T* in) const;

		//! Transforms the vector by this matrix as though it was in 2D (Z ignored).
		void TransformVect2D(FVec3<T>& vect) const;

		//! Transforms input vector by this matrix and stores result in output vector as though it was in 2D (Z ignored).
		void TransformVect2D(FVec3<T>& out, const FVec3<T>& in) const;

		//! An alternate transform vector method, writing into an array of 4 floats
		void TransformVect(T* out, const FVec3<T>& in) const;

		//! Translate a vector by the translation part of this matrix.
		void TranslateVect(FVec3<T>& vect) const;

		//! Transforms a plane by this matrix
		void TransformPlane(FPlane3D<T>& plane) const;

		//! Transforms a plane by this matrix ( some problems to solve..)
		void TransformPlane_new(FPlane3D<T>& plane) const;

		//! Transforms a plane by this matrix
		void TransformPlane(const FPlane3D<T>& in, FPlane3D<T>& out) const;

		//! Transforms a axis aligned bounding box
		/** The result box of this operation may not be accurate at all. For
			correct results, use transformBoxEx() */
		void TransformBox(FAABBox<T>& box) const;

		//! Transforms a axis aligned bounding box
		/** The result box of this operation should by accurate, but this operation
			is slower than transformBox(). */
		void TransformBoxEx(FAABBox<T>& box) const;

		//! Transforms a axis aligned bounding box from UE4 FBox::TransformBy(const FMatrix& M)
		/** Faster than  transformBoxEx() in release version;
		*/
		void TransformBoxUE(FAABBox<T>& box) const;

		//! Multiplies this matrix by a 1x4 matrix
		void MultiplyWith1x4Matrix(T* matrix) const;

		//! Calculates inverse of matrix. Slow.
		/** \return Returns false if there is no inverse matrix.*/
		bool MakeInverse();

		//! Computes the determinant of the matrix.
		T GetDeterminant() const;

		//! Inverts a primitive matrix which only contains a translation and a rotation
		/** \param out where result matrix is written to. */
		bool GetInversePrimitive(FMatrix4<T>& out) const;

		//! Gets the inversed matrix of this one
		/** \param out where result matrix is written to.
			\return Returns false if there is no inverse matrix. */
		bool GetInverse(FMatrix4<T>& out) const;

		//! Creates a newly matrix as interpolated matrix from two other ones.
		/** \param b other matrix to interpolate with
			\param time Must be a value between 0 and 1. */
		FMatrix4<T> Interpolate(const FMatrix4<T>& b, T time) const;

		//! Gets transposed matrix
		FMatrix4<T> GetTransposed() const;

		//! Gets transposed matrix
		inline void GetTransposed(FMatrix4<T>& dest) const;

		//! Set texture transformation rotation
		/** Rotate about z axis, recenter at (0.5,0.5).
			Doesn't clear other elements than those affected
			\param radAngle Angle in radians
			\return Altered matrix */
		FMatrix4<T>& SetTextureRotationCenter(T radAngle);

		//! Set texture transformation translation
		/** Doesn't clear other elements than those affected.
			\param x Offset on x axis
			\param y Offset on y axis
			\return Altered matrix */
		FMatrix4<T>& SetTextureTranslate(T x, T y);

		//! Set texture transformation translation, using a transposed representation
		/** Doesn't clear other elements than those affected.
			\param x Offset on x axis
			\param y Offset on y axis
			\return Altered matrix */
		FMatrix4<T>& SetTextureTranslateTransposed(T x, T y);

		//! Set texture transformation scale
		/** Doesn't clear other elements than those affected.
			\param sx Scale factor on x axis
			\param sy Scale factor on y axis
			\return Altered matrix. */
		FMatrix4<T>& SetTextureScale(T sx, T sy);

		//! Set texture transformation scale, and recenter at (0.5,0.5)
		/** Doesn't clear other elements than those affected.
			\param sx Scale factor on x axis
			\param sy Scale factor on y axis
			\return Altered matrix. */
		FMatrix4<T>& SetTextureScaleCenter(T sx, T sy);

		//! Applies a texture post scale.
		/**	\param sx Scale factor on x axis
			\param sy Scale factor on y axis
			\return Altered matrix. */
		FMatrix4<T>& PostTextureScale(T sx, T sy);

		//! Sets all matrix data members at once
		FMatrix4<T>& SetM(const T* data);

		//! Gets all matrix data members at once
		/** \returns data */
		T* GetM(T* data) const;

	private:
		//! Matrix data, stored in row-major order
		T M[16];
	};

	// Default constructor
	template <class T>
	inline FMatrix4<T>::FMatrix4(eConstructor constructor)
	{
		switch (constructor)
		{
		case EM4CONST_NOTHING:
		case EM4CONST_COPY:
			break;
		case EM4CONST_IDENTITY:
		case EM4CONST_INVERSE:
		default:
			MakeIdentity();
			break;
		}
	}

	//! Contructor with data
	template <class T>
	inline FMatrix4<T>::FMatrix4(
		T _00, T _01, T _02, T _03,
		T _10, T _11, T _12, T _13,
		T _20, T _21, T _22, T _23,
		T _30, T _31, T _32, T _33)
	{
		M[0] = _00;
		M[1] = _01;
		M[2] = _02;
		M[3] = _03;

		M[4] = _10;
		M[5] = _11;
		M[6] = _12;
		M[7] = _13;

		M[8] = _20;
		M[9] = _21;
		M[10] = _22;
		M[11] = _23;

		M[12] = _30;
		M[13] = _31;
		M[14] = _32;
		M[15] = _33;
	}

	// Copy constructor
	template <class T>
	inline FMatrix4<T>::FMatrix4(const FMatrix4<T>& other, eConstructor constructor)
	{
		switch (constructor)
		{
		case EM4CONST_IDENTITY:
			MakeIdentity();
			break;
		case EM4CONST_NOTHING:
			break;
		case EM4CONST_COPY:
			*this = other;
			break;
		case EM4CONST_TRANSPOSED:
			other.GetTransposed(*this);
			break;
		case EM4CONST_INVERSE:
			if (!other.GetInverse(*this))
				memset(M, 0, 16 * sizeof(T));
			break;
		case EM4CONST_INVERSE_TRANSPOSED:
			if (!other.GetInverse(*this))
				memset(M, 0, 16 * sizeof(T));
			else
				*this = GetTransposed();
			break;
		}
	}

	//! Add another matrix.
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::operator+(const FMatrix4<T>& other) const
	{
		FMatrix4<T> temp(EM4CONST_NOTHING);

		temp[0] = M[0] + other[0];
		temp[1] = M[1] + other[1];
		temp[2] = M[2] + other[2];
		temp[3] = M[3] + other[3];
		temp[4] = M[4] + other[4];
		temp[5] = M[5] + other[5];
		temp[6] = M[6] + other[6];
		temp[7] = M[7] + other[7];
		temp[8] = M[8] + other[8];
		temp[9] = M[9] + other[9];
		temp[10] = M[10] + other[10];
		temp[11] = M[11] + other[11];
		temp[12] = M[12] + other[12];
		temp[13] = M[13] + other[13];
		temp[14] = M[14] + other[14];
		temp[15] = M[15] + other[15];

		return temp;
	}

	//! Add another matrix.
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator+=(const FMatrix4<T>& other)
	{
		M[0] += other[0];
		M[1] += other[1];
		M[2] += other[2];
		M[3] += other[3];
		M[4] += other[4];
		M[5] += other[5];
		M[6] += other[6];
		M[7] += other[7];
		M[8] += other[8];
		M[9] += other[9];
		M[10] += other[10];
		M[11] += other[11];
		M[12] += other[12];
		M[13] += other[13];
		M[14] += other[14];
		M[15] += other[15];

		return *this;
	}

	//! Subtract another matrix.
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::operator-(const FMatrix4<T>& other) const
	{
		FMatrix4<T> temp(EM4CONST_NOTHING);

		temp[0] = M[0] - other[0];
		temp[1] = M[1] - other[1];
		temp[2] = M[2] - other[2];
		temp[3] = M[3] - other[3];
		temp[4] = M[4] - other[4];
		temp[5] = M[5] - other[5];
		temp[6] = M[6] - other[6];
		temp[7] = M[7] - other[7];
		temp[8] = M[8] - other[8];
		temp[9] = M[9] - other[9];
		temp[10] = M[10] - other[10];
		temp[11] = M[11] - other[11];
		temp[12] = M[12] - other[12];
		temp[13] = M[13] - other[13];
		temp[14] = M[14] - other[14];
		temp[15] = M[15] - other[15];

		return temp;
	}

	//! Subtract another matrix.
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator-=(const FMatrix4<T>& other)
	{
		M[0] -= other[0];
		M[1] -= other[1];
		M[2] -= other[2];
		M[3] -= other[3];
		M[4] -= other[4];
		M[5] -= other[5];
		M[6] -= other[6];
		M[7] -= other[7];
		M[8] -= other[8];
		M[9] -= other[9];
		M[10] -= other[10];
		M[11] -= other[11];
		M[12] -= other[12];
		M[13] -= other[13];
		M[14] -= other[14];
		M[15] -= other[15];

		return *this;
	}

	//! Multiply by scalar.
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::operator*(const T& scalar) const
	{
		FMatrix4<T> temp(EM4CONST_NOTHING);

		temp[0] = M[0] * scalar;
		temp[1] = M[1] * scalar;
		temp[2] = M[2] * scalar;
		temp[3] = M[3] * scalar;
		temp[4] = M[4] * scalar;
		temp[5] = M[5] * scalar;
		temp[6] = M[6] * scalar;
		temp[7] = M[7] * scalar;
		temp[8] = M[8] * scalar;
		temp[9] = M[9] * scalar;
		temp[10] = M[10] * scalar;
		temp[11] = M[11] * scalar;
		temp[12] = M[12] * scalar;
		temp[13] = M[13] * scalar;
		temp[14] = M[14] * scalar;
		temp[15] = M[15] * scalar;

		return temp;
	}

	//! Multiply by scalar.
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator*=(const T& scalar)
	{
		M[0] *= scalar;
		M[1] *= scalar;
		M[2] *= scalar;
		M[3] *= scalar;
		M[4] *= scalar;
		M[5] *= scalar;
		M[6] *= scalar;
		M[7] *= scalar;
		M[8] *= scalar;
		M[9] *= scalar;
		M[10] *= scalar;
		M[11] *= scalar;
		M[12] *= scalar;
		M[13] *= scalar;
		M[14] *= scalar;
		M[15] *= scalar;

		return *this;
	}

	//! Multiply by another matrix.
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator*=(const FMatrix4<T>& other)
	{
		FMatrix4<T> temp(*this);
		return Setbyproduct_nocheck(temp, other);
	}

	//! multiply by another matrix
	// set this matrix to the product of two other matrices
	// goal is to reduce stack use and copy
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::Setbyproduct_nocheck(const FMatrix4<T>& other_a, const FMatrix4<T>& other_b)
	{
		const T* m1 = other_a.M;
		const T* m2 = other_b.M;

		RowMatrixProduct(M, m1, m2);
		return *this;
	}


	//! multiply by another matrix
	// set this matrix to the product of two other matrices
	// goal is to reduce stack use and copy
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::Setbyproduct(const FMatrix4<T>& other_a, const FMatrix4<T>& other_b)
	{
		Setbyproduct_nocheck(other_a, other_b);
	}

	//! multiply by another matrix
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::operator*(const FMatrix4<T>& m) const
	{
		FMatrix4<T> m3(EM4CONST_NOTHING);

#if TI_USE_RH
		const T* m1 = M;
		const T* m2 = m.M;
#else
		const T* m2 = M;
		const T* m1 = m.M;
#endif

		m3[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
		m3[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
		m3[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
		m3[3] = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];

		m3[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
		m3[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
		m3[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
		m3[7] = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];

		m3[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
		m3[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
		m3[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
		m3[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];

		m3[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
		m3[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
		m3[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
		m3[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
		return m3;
	}

	template <class T>
	inline FMatrix4<T> FMatrix4<T>::Mult34(const FMatrix4<T>& m2) const
	{
		FMatrix4<T> out;
		Mult34(m2, out);
		return out;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::Mult34(const FMatrix4<T>& m, FMatrix4<T>& out) const
	{
#if TI_USE_RH
		const T* m1 = M;
		const T* m2 = m.M;
#else
		const T* m2 = M;
		const T* m1 = m.M;
#endif

		out.M[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2];
		out.M[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2];
		out.M[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2];
		out.M[3] = 0.0f;

		out.M[4] = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6];
		out.M[5] = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6];
		out.M[6] = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6];
		out.M[7] = 0.0f;

		out.M[8] = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10];
		out.M[9] = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10];
		out.M[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10];
		out.M[11] = 0.0f;

		out.M[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12];
		out.M[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13];
		out.M[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14];
		out.M[15] = 1.0f;

		return out;
	}

	template <class T>
	inline FVec3<T> FMatrix4<T>::GetColumn(uint32 c) const
	{
		const T* v = &M[c * 4];
		return FVec3<T>(v[0], v[1], v[2]);
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetColumn(uint32 c, const FVec3<T>& v)
	{
		T* dst = &M[c * 4];
		dst[0] = v.X;
		dst[1] = v.Y;
		dst[2] = v.Z;
		return *this;
	}

	template <class T>
	inline FVec3<T> FMatrix4<T>::GetTranslation() const
	{
		return FVec3<T>(M[12], M[13], M[14]);
	}


	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTranslation(const FVec3<T>& translation)
	{
		M[12] = translation.X;
		M[13] = translation.Y;
		M[14] = translation.Z;
		return *this;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetScale(const FVec3<T>& scale)
	{
		M[0] = scale.X;
		M[5] = scale.Y;
		M[10] = scale.Z;
		return *this;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::PreScale(const FVec3<T>& scale)
	{
		M[0] *= scale.X;
		M[1] *= scale.Y;
		M[2] *= scale.Z;

		M[4] *= scale.X;
		M[5] *= scale.Y;
		M[6] *= scale.Z;

		M[8] *= scale.X;
		M[9] *= scale.Y;
		M[10] *= scale.Z;

		M[12] *= scale.X;
		M[13] *= scale.Y;
		M[14] *= scale.Z;
		return *this;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::PostScale(const FVec3<T>& scale)
	{
		M[0] *= scale.X;
		M[1] *= scale.X;
		M[2] *= scale.X;

		M[4] *= scale.Y;
		M[5] *= scale.Y;
		M[6] *= scale.Y;

		M[8] *= scale.Z;
		M[9] *= scale.Z;
		M[10] *= scale.Z;
		return *this;
	}

	template <class T>
	inline FVec3<T> FMatrix4<T>::GetScale() const
	{
		FVec3<T> vScale;
		vScale.X = FVec3<T>(M[0], M[1], M[2]).GetLength();
		vScale.Y = FVec3<T>(M[4], M[5], M[6]).GetLength();
		vScale.Z = FVec3<T>(M[8], M[9], M[10]).GetLength();
		return vScale;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetRotationRadians(const FVec3<T>& rotation)
	{
		const float64 cr = cos(rotation.X);
		const float64 sr = sin(rotation.X);
		const float64 cp = cos(rotation.Y);
		const float64 sp = sin(rotation.Y);
		const float64 cy = cos(rotation.Z);
		const float64 sy = sin(rotation.Z);

		M[0] = (T)(cp * cy);
		M[1] = (T)(cp * sy);
		M[2] = (T)(-sp);

		const float64 srsp = sr * sp;
		const float64 crsp = cr * sp;

		M[4] = (T)(srsp * cy - cr * sy);
		M[5] = (T)(srsp * sy + cr * cy);
		M[6] = (T)(sr * cp);

		M[8] = (T)(crsp * cy + sr * sy);
		M[9] = (T)(crsp * sy - sr * cy);
		M[10] = (T)(cr * cp);

		return *this;
	}


	//! Returns the rotation, as set by setRotation(). This code was sent
	//! in by Chev.
	template <class T>
	inline FVec3<T> FMatrix4<T>::GetRotationRadians() const
	{
		const FMatrix4<T>& mat = *this;

		float Y = -asin(mat(0, 2));
		const float C = cos(Y);

		float rotx, roty, X, Z;

		if (fabs(C) > ROUNDING_ERROR_64)
		{
			const T invC = (T)(1.0 / C);
			rotx = mat(2, 2) * invC;
			roty = mat(1, 2) * invC;
			X = atan2(roty, rotx);
			rotx = mat(0, 0) * invC;
			roty = mat(0, 1) * invC;
			Z = atan2(roty, rotx);
		}
		else
		{
			X = 0.0;
			rotx = mat(1, 1);
			roty = -mat(1, 0);
			Z = atan2(roty, rotx);
		}

		if (X < 0.0) X += PI * 2.f;
		if (Y < 0.0) Y += PI * 2.f;
		if (Z < 0.0) Z += PI * 2.f;

		return FVec3<T>((T)X, (T)Y, (T)Z);
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::MakeIdentity()
	{
		memset(M, 0, 16 * sizeof(T));
		M[0] = M[5] = M[10] = M[15] = (T)1;
		return *this;
	}


	template <class T>
	inline void FMatrix4<T>::RotateVect(FVec3<T>& vect) const
	{
		FVec3<T> tmp = vect;
		vect.X = tmp.X * M[0] + tmp.Y * M[4] + tmp.Z * M[8];
		vect.Y = tmp.X * M[1] + tmp.Y * M[5] + tmp.Z * M[9];
		vect.Z = tmp.X * M[2] + tmp.Y * M[6] + tmp.Z * M[10];
	}

	//! An alternate transform vector method, writing into a second vector
	template <class T>
	inline void FMatrix4<T>::RotateVect(FVec3<T>& out, const FVec3<T>& in) const
	{
		out.X = in.X * M[0] + in.Y * M[4] + in.Z * M[8];
		out.Y = in.X * M[1] + in.Y * M[5] + in.Z * M[9];
		out.Z = in.X * M[2] + in.Y * M[6] + in.Z * M[10];
	}

	//! An alternate transform vector method, writing into an array of 3 floats
	template <class T>
	inline void FMatrix4<T>::RotateVect(T* out, const FVec3<T>& in) const
	{
		out[0] = in.X * M[0] + in.Y * M[4] + in.Z * M[8];
		out[1] = in.X * M[1] + in.Y * M[5] + in.Z * M[9];
		out[2] = in.X * M[2] + in.Y * M[6] + in.Z * M[10];
	}

	template <class T>
	inline void FMatrix4<T>::InverseRotateVect(FVec3<T>& vect) const
	{
		FVec3<T> tmp = vect;
		vect.X = tmp.X * M[0] + tmp.Y * M[1] + tmp.Z * M[2];
		vect.Y = tmp.X * M[4] + tmp.Y * M[5] + tmp.Z * M[6];
		vect.Z = tmp.X * M[8] + tmp.Y * M[9] + tmp.Z * M[10];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect(FVec3<T>& vect) const
	{
		T vector[3];

		vector[0] = vect.X * M[0] + vect.Y * M[4] + vect.Z * M[8] + M[12];
		vector[1] = vect.X * M[1] + vect.Y * M[5] + vect.Z * M[9] + M[13];
		vector[2] = vect.X * M[2] + vect.Y * M[6] + vect.Z * M[10] + M[14];

		vect.X = vector[0];
		vect.Y = vector[1];
		vect.Z = vector[2];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect2D(FVec3<T>& vect) const
	{
		T vector[2];

		vector[0] = vect.X * M[0] + vect.Y * M[4] + M[12];
		vector[1] = vect.X * M[1] + vect.Y * M[5] + M[13];

		vect.X = vector[0];
		vect.Y = vector[1];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect2D(FVec3<T>& out, const FVec3<T>& in) const
	{
		out.X = in.X * M[0] + in.Y * M[4] + M[12];
		out.Y = in.X * M[1] + in.Y * M[5] + M[13];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect(FVec3<T>& out, const FVec3<T>& in) const
	{
		out.X = in.X * M[0] + in.Y * M[4] + in.Z * M[8] + M[12];
		out.Y = in.X * M[1] + in.Y * M[5] + in.Z * M[9] + M[13];
		out.Z = in.X * M[2] + in.Y * M[6] + in.Z * M[10] + M[14];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect(FVec3<T>& out, const T* in) const
	{
		out.X = in[0] * M[0] + in[1] * M[4] + in[2] * M[8] + M[12];
		out.Y = in[0] * M[1] + in[1] * M[5] + in[2] * M[9] + M[13];
		out.Z = in[0] * M[2] + in[1] * M[6] + in[2] * M[10] + M[14];
	}

	template <class T>
	inline void FMatrix4<T>::TransformVect(T* out, const FVec3<T>& in) const
	{
		out[0] = in.X * M[0] + in.Y * M[4] + in.Z * M[8] + M[12];
		out[1] = in.X * M[1] + in.Y * M[5] + in.Z * M[9] + M[13];
		out[2] = in.X * M[2] + in.Y * M[6] + in.Z * M[10] + M[14];
		out[3] = in.X * M[3] + in.Y * M[7] + in.Z * M[11] + M[15];
	}

	//! Transforms a axis aligned bounding box
	template <class T>
	inline void FMatrix4<T>::TransformBox(FAABBox<T>& box) const
	{
		TransformVect(box.Min);
		TransformVect(box.Max);
		box.repair();
	}

	//! Transforms a axis aligned bounding box more accurately than transformBox()
	template <class T>
	inline void FMatrix4<T>::TransformBoxEx(FAABBox<T>& box) const
	{
		const T Amin[3] = { box.Min.X, box.Min.Y, box.Min.Z };
		const T Amax[3] = { box.Max.X, box.Max.Y, box.Max.Z };

		T Bmin[3];
		T Bmax[3];

		Bmin[0] = Bmax[0] = M[12];
		Bmin[1] = Bmax[1] = M[13];
		Bmin[2] = Bmax[2] = M[14];

		const FMatrix4<T>& m = *this;

		for (uint32 i = 0; i < 3; ++i)
		{
			for (uint32 j = 0; j < 3; ++j)
			{
				const T a = m(j, i) * Amin[j];
				const T b = m(j, i) * Amax[j];

				if (a < b)
				{
					Bmin[i] += a;
					Bmax[i] += b;
				}
				else
				{
					Bmin[i] += b;
					Bmax[i] += a;
				}
			}
		}

		box.Min.X = Bmin[0];
		box.Min.Y = Bmin[1];
		box.Min.Z = Bmin[2];

		box.Max.X = Bmax[0];
		box.Max.Y = Bmax[1];
		box.Max.Z = Bmax[2];
	}

	template<typename T>
	inline FVec3<T> ti_vec_abs(const FVec3<T>& vec)
	{
		FVec3<T> new_vec;
		new_vec.X = TMath::Abs(vec.X);
		new_vec.Y = TMath::Abs(vec.Y);
		new_vec.Z = TMath::Abs(vec.Z);
		return new_vec;
	}

	//! Transforms a axis aligned bounding box from UE4 FBox::TransformBy(const FMatrix& M)
	template <class T>
	inline void FMatrix4<T>::TransformBoxUE(FAABBox<T>& box) const
	{
		FVec3<T> Origin = box.GetCenter();
		FVec3<T> Extent = box.GetExtent() * 0.5f;

		FVec3<T> Rot0(M[0], M[1], M[2]);
		FVec3<T> Rot1(M[4], M[5], M[6]);
		FVec3<T> Rot2(M[8], M[9], M[10]);
		FVec3<T> Trans(M[12], M[13], M[14]);

		FVec3<T> NewOrigin = FVec3<T>(Origin.X, Origin.X, Origin.X) * Rot0;
		NewOrigin = FVec3<T>(Origin.Y, Origin.Y, Origin.Y) * Rot1 + NewOrigin;
		NewOrigin = FVec3<T>(Origin.Z, Origin.Z, Origin.Z) * Rot2 + NewOrigin;
		NewOrigin = NewOrigin + Trans;

		FVec3<T> NewExtent = ti_vec_abs<T>(FVec3<T>(Extent.X, Extent.X, Extent.X) * Rot0);
		NewExtent = NewExtent + ti_vec_abs<T>(FVec3<T>(Extent.Y, Extent.Y, Extent.Y) * Rot1);
		NewExtent = NewExtent + ti_vec_abs<T>(FVec3<T>(Extent.Z, Extent.Z, Extent.Z) * Rot2);

		box.Min = NewOrigin - NewExtent;
		box.Max = NewOrigin + NewExtent;
	}

	//! Multiplies this matrix by a 1x4 matrix
	template <class T>
	inline void FMatrix4<T>::MultiplyWith1x4Matrix(T* matrix) const
	{
		/*
		  0  1  2  3
		  4  5  6  7
		  8  9  10 11
		  12 13 14 15
		*/

		T mat[4];
		mat[0] = matrix[0];
		mat[1] = matrix[1];
		mat[2] = matrix[2];
		mat[3] = matrix[3];

		matrix[0] = M[0] * mat[0] + M[4] * mat[1] + M[8] * mat[2] + M[12] * mat[3];
		matrix[1] = M[1] * mat[0] + M[5] * mat[1] + M[9] * mat[2] + M[13] * mat[3];
		matrix[2] = M[2] * mat[0] + M[6] * mat[1] + M[10] * mat[2] + M[14] * mat[3];
		matrix[3] = M[3] * mat[0] + M[7] * mat[1] + M[11] * mat[2] + M[15] * mat[3];
	}

	template <class T>
	inline void FMatrix4<T>::InverseTranslateVect(FVec3<T>& vect) const
	{
		vect.X = vect.X - M[12];
		vect.Y = vect.Y - M[13];
		vect.Z = vect.Z - M[14];
	}

	template <class T>
	inline void FMatrix4<T>::TranslateVect(FVec3<T>& vect) const
	{
		vect.X = vect.X + M[12];
		vect.Y = vect.Y + M[13];
		vect.Z = vect.Z + M[14];
	}

	//! Transforms a plane by this matrix
	template <class T>
	inline void FMatrix4<T>::TransformPlane(FPlane3D<T>& plane) const
	{
		FVec3<T> member;
		TransformVect(member, plane.GetMemberPoint());

		FVec3<T> origin(0, 0, 0);
		TransformVect(plane.Normal);
		TransformVect(origin);

		plane.Normal -= origin;
		plane.D = -member.Dot(plane.Normal);
	}

	//! Transforms a plane by this matrix
	template <class T>
	inline void FMatrix4<T>::TransformPlane_new(FPlane3D<T>& plane) const
	{
		// rotate normal -> rotateVect ( plane.n );
		FVec3<T> n;
		n.X = plane.Normal.X * M[0] + plane.Normal.Y * M[4] + plane.Normal.Z * M[8];
		n.Y = plane.Normal.X * M[1] + plane.Normal.Y * M[5] + plane.Normal.Z * M[9];
		n.Z = plane.Normal.X * M[2] + plane.Normal.Y * M[6] + plane.Normal.Z * M[10];

		// compute newly d. -> GetTranslation(). dotproduct ( plane.n )
		plane.D -= M[12] * n.X + M[13] * n.Y + M[14] * n.Z;
		plane.Normal.X = n.X;
		plane.Normal.Y = n.Y;
		plane.Normal.Z = n.Z;
	}

	//! Transforms a plane by this matrix
	template <class T>
	inline void FMatrix4<T>::TransformPlane(const FPlane3D<T>& in, FPlane3D<T>& out) const
	{
		out = in;
		TransformPlane(out);
	}

	template <class T>
	inline T FMatrix4<T>::GetDeterminant() const
	{
		T t0 = M[10] * M[15] - M[11] * M[14];
		T t1 = M[6] * M[15] - M[7] * M[14];
		T t2 = M[6] * M[11] - M[7] * M[10];
		T t3 = M[2] * M[15] - M[3] * M[14];
		T t4 = M[2] * M[11] - M[3] * M[10];
		T t5 = M[2] * M[7] - M[3] * M[6];

		T t6 = M[8] * M[13] - M[9] * M[12];
		T t7 = M[4] * M[13] - M[5] * M[12];
		T t8 = M[4] * M[9] - M[5] * M[8];
		T t9 = M[0] * M[13] - M[1] * M[12];
		T t10 = M[0] * M[9] - M[1] * M[8];
		T t11 = M[0] * M[5] - M[1] * M[4];

		return t0 * t11 - t1 * t10 + t2 * t9 + t3 * t8 - t4 * t7 + t5 * t6;
	}

	template <class T>
	inline bool FMatrix4<T>::GetInverse(FMatrix4<T>& out) const
	{
		// Cramer's rule.
		T t0 = M[10] * M[15] - M[11] * M[14];
		T t1 = M[6] * M[15] - M[7] * M[14];
		T t2 = M[6] * M[11] - M[7] * M[10];
		T t3 = M[2] * M[15] - M[3] * M[14];
		T t4 = M[2] * M[11] - M[3] * M[10];
		T t5 = M[2] * M[7] - M[3] * M[6];

		T t6 = M[8] * M[13] - M[9] * M[12];
		T t7 = M[4] * M[13] - M[5] * M[12];
		T t8 = M[4] * M[9] - M[5] * M[8];
		T t9 = M[0] * M[13] - M[1] * M[12];
		T t10 = M[0] * M[9] - M[1] * M[8];
		T t11 = M[0] * M[5] - M[1] * M[4];

		T det = t0 * t11 - t1 * t10 + t2 * t9 + t3 * t8 - t4 * t7 + t5 * t6;
		if (TMath::IsZero(det))
		{
			return false;
		}

		out.M[0] = M[5] * t0 - M[9] * t1 + M[13] * t2;
		out.M[1] = M[9] * t3 - M[1] * t0 - M[13] * t4;
		out.M[2] = M[1] * t1 - M[5] * t3 + M[13] * t5;
		out.M[3] = M[5] * t4 - M[1] * t2 - M[9] * t5;

		out.M[4] = M[8] * t1 - M[4] * t0 - M[12] * t2;
		out.M[5] = M[0] * t0 - M[8] * t3 + M[12] * t4;
		out.M[6] = M[4] * t3 - M[0] * t1 - M[12] * t5;
		out.M[7] = M[0] * t2 - M[4] * t4 + M[8] * t5;

		out.M[8] = M[7] * t6 - M[11] * t7 + M[15] * t8;
		out.M[9] = M[11] * t9 - M[3] * t6 - M[15] * t10;
		out.M[10] = M[3] * t7 - M[7] * t9 + M[15] * t11;
		out.M[11] = M[7] * t10 - M[3] * t8 - M[11] * t11;

		out.M[12] = M[10] * t7 - M[6] * t6 - M[14] * t8;
		out.M[13] = M[2] * t6 - M[10] * t9 + M[14] * t10;
		out.M[14] = M[6] * t9 - M[2] * t7 - M[14] * t11;
		out.M[15] = M[2] * t8 - M[6] * t10 + M[10] * t11;

		//det = reciprocal(det);
		det = 1.0f / det;
		for (int i = 0; i < 16; ++i)
		{
			out.M[i] *= det;
		}

		return true;
	}

	//! Inverts a primitive matrix which only contains a translation and a rotation
	//! \param out: where result matrix is written to.
	template <class T>
	inline bool FMatrix4<T>::GetInversePrimitive(FMatrix4<T>& out) const
	{
		out.M[0] = M[0];
		out.M[1] = M[4];
		out.M[2] = M[8];
		out.M[3] = 0;

		out.M[4] = M[1];
		out.M[5] = M[5];
		out.M[6] = M[9];
		out.M[7] = 0;

		out.M[8] = M[2];
		out.M[9] = M[6];
		out.M[10] = M[10];
		out.M[11] = 0;

		out.M[12] = -(M[12] * M[0] + M[13] * M[1] + M[14] * M[2]);
		out.M[13] = -(M[12] * M[4] + M[13] * M[5] + M[14] * M[6]);
		out.M[14] = -(M[12] * M[8] + M[13] * M[9] + M[14] * M[10]);
		out.M[15] = T(1);

		return true;
	}

	/*!
	 */
	template <class T>
	inline bool FMatrix4<T>::MakeInverse()
	{
		FMatrix4<T> temp(EM4CONST_NOTHING);

		if (GetInverse(temp))
		{
			*this = temp;
			return true;
		}

		return false;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator=(const T& scalar)
	{
		for (int i = 0; i < 16; ++i)
			M[i] = scalar;

		return *this;
	}


	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::operator=(const FMatrix4<T>& other)
	{
		for (int i = 0; i < 16; ++i)
			M[i] = other.M[i];

		return *this;
	}


	template <class T>
	inline bool FMatrix4<T>::operator==(const FMatrix4<T>& other) const
	{
		for (int i = 0; i < 16; ++i)
			if (M[i] != other.M[i])
				return false;

		return true;
	}


	template <class T>
	inline bool FMatrix4<T>::operator!=(const FMatrix4<T>& other) const
	{
		return !(*this == other);
	}

	// creates a newly matrix as interpolated matrix from this and the passed one.
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::Interpolate(const FMatrix4<T>& b, T time) const
	{
		FMatrix4<T> mat(EM4CONST_NOTHING);

		for (uint32 i = 0; i < 16; i += 4)
		{
			mat.M[i + 0] = (T)(M[i + 0] + (b.M[i + 0] - M[i + 0]) * time);
			mat.M[i + 1] = (T)(M[i + 1] + (b.M[i + 1] - M[i + 1]) * time);
			mat.M[i + 2] = (T)(M[i + 2] + (b.M[i + 2] - M[i + 2]) * time);
			mat.M[i + 3] = (T)(M[i + 3] + (b.M[i + 3] - M[i + 3]) * time);
		}
		return mat;
	}


	// returns transposed matrix
	template <class T>
	inline FMatrix4<T> FMatrix4<T>::GetTransposed() const
	{
		FMatrix4<T> t(EM4CONST_NOTHING);
		GetTransposed(t);
		return t;
	}


	// returns transposed matrix
	template <class T>
	inline void FMatrix4<T>::GetTransposed(FMatrix4<T>& o) const
	{
		o.M[0] = M[0];
		o.M[1] = M[4];
		o.M[2] = M[8];
		o.M[3] = M[12];

		o.M[4] = M[1];
		o.M[5] = M[5];
		o.M[6] = M[9];
		o.M[7] = M[13];

		o.M[8] = M[2];
		o.M[9] = M[6];
		o.M[10] = M[10];
		o.M[11] = M[14];

		o.M[12] = M[3];
		o.M[13] = M[7];
		o.M[14] = M[11];
		o.M[15] = M[15];
	}

	// rotate about z axis, center ( 0.5, 0.5 )
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTextureRotationCenter(T rotateRad)
	{
		const T c = cosf(rotateRad);
		const T s = sinf(rotateRad);
		M[0] = (T)c;
		M[1] = (T)s;

		M[4] = (T)-s;
		M[5] = (T)c;

		M[8] = (T)(0.5f * (s - c) + 0.5f);
		M[9] = (T)(-0.5f * (s + c) + 0.5f);

		return *this;
	}


	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTextureTranslate(T x, T y)
	{
		M[8] = (T)x;
		M[9] = (T)y;

		return *this;
	}


	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTextureTranslateTransposed(T x, T y)
	{
		M[2] = (T)x;
		M[6] = (T)y;

		return *this;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTextureScale(T sx, T sy)
	{
		M[0] = (T)sx;
		M[5] = (T)sy;

		return *this;
	}

	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::PostTextureScale(T sx, T sy)
	{
		M[0] *= (T)sx;
		M[1] *= (T)sx;
		M[4] *= (T)sy;
		M[5] *= (T)sy;

		return *this;
	}


	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetTextureScaleCenter(T sx, T sy)
	{
		M[0] = (T)sx;
		M[5] = (T)sy;
		M[8] = (T)(0.5f - 0.5f * sx);
		M[9] = (T)(0.5f - 0.5f * sy);

		return *this;
	}


	// sets all matrix data members at once
	template <class T>
	inline FMatrix4<T>& FMatrix4<T>::SetM(const T* data)
	{
		memcpy(M, data, 16 * sizeof(T));

		return *this;
	}

	// gets all matrix data members at once
	template <class T>
	inline T* FMatrix4<T>::GetM(T* data) const
	{
		memcpy(data, M, 16 * sizeof(T));
		return data;
	}

	template <class T>
	inline FMatrix4<T> operator*(const T scalar, const FMatrix4<T>& mat)
	{
		return mat * scalar;
	}

	typedef FMatrix4<float32> FMat4;
	//extern const FMat4 IdentityMatrix;

	//MT must have operator[]
	template<class T, typename MT>
	inline FMatrix4<T> operator*(const FMatrix4<T>& mat, const MT& other)
	{
		FMatrix4<T> mat2(FMatrix4<T>::EM4CONST_NOTHING);

		if (mat.getDefinitelyIdentityMatrix())
		{
			for (uint32 i = 0; i < 16; ++i)
			{
				mat2[i] = other[i];
			}
		}
		else
		{
			RowMatrixProduct34(mat2, mat, other.M);
		}

		return mat2;
	}

	//MT must have operator[]
	template<typename T, typename MT>
	inline FMatrix4<T> operator*(const MT& other, const FMatrix4<T>& mat)
	{
		FMatrix4<T> mat2(FMatrix4<T>::EM4CONST_NOTHING);

		FMatrix4<T>::RowMatrixProduct34(mat2, other.M, mat);

		return mat2;
	}


	//! Builds a right-handed perspective projection matrix based on a field of view
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixPerspectiveFov(T fieldOfViewRadians, T aspectRatio, T zNear, T zFar)
	{
		const float64 h = 1.0 / tan(fieldOfViewRadians / 2.0);
		const T w = (T)(h / aspectRatio);

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = w;
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)h;
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = (T)(zFar / (zFar - zNear));
		m(2, 3) = 1;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)(-zNear * zFar / (zFar - zNear));
		m(3, 3) = 0;

		return m;
	}

	//! Builds a reversed right-handed perspective projection matrix based on a field of view
	template<typename T>
	FMatrix4<T> BuildReversedProjectionMatrixPerspectiveFov(T fieldOfViewRadians, T aspectRatio, T zNear, T zFar)
	{
		const float64 h = 1.0 / tan(fieldOfViewRadians / 2.0);
		const T w = (T)(h / aspectRatio);

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = w;
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)h;
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = (T)(zNear / (zNear - zFar));
		m(2, 3) = 1;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)(-zFar * zNear / (zNear - zFar));
		m(3, 3) = 0;

		return m;
	}

	//! Builds a left-handed perspective projection matrix.
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixPerspectiveFovInfinity(T fieldOfViewRadians, T aspectRatio, T zNear)
	{
		const float64 h = 1.0 / tan(fieldOfViewRadians / 2.0);
		const T w = (T)(h / aspectRatio);

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = w;
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)h;
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = 1;
		m(2, 3) = 1;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)-zNear;
		m(3, 3) = 0;

		return m;
	}

	//! Builds a left-handed perspective projection matrix.
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixPerspective(T widthOfViewVolume, T heightOfViewVolume, T zNear, T zFar)
	{
		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = (T)(2 * zNear / widthOfViewVolume);
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)(2 * zNear / heightOfViewVolume);
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = (T)(zFar / (zFar - zNear));
		m(2, 3) = 1;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)(zNear * zFar / (zNear - zFar));
		m(3, 3) = 0;

		return m;
	}

	//! Builds a left-handed perspective projection matrix.
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixPerspectiveInfinity(T widthOfViewVolume, T heightOfViewVolume, T zNear)
	{
		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = (T)(2 * zNear / widthOfViewVolume);
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)(2 * zNear / heightOfViewVolume);
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = 1;
		m(2, 3) = 1;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)-zNear;
		m(3, 3) = 0;

		return m;
	}

	//! Builds a centered right-handed orthogonal projection matrix.
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixOrtho(T widthOfViewVolume, T heightOfViewVolume, T zNear, T zFar)
	{

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = (T)(2 / widthOfViewVolume);
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)(2 / heightOfViewVolume);
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = (T)(1 / (zFar - zNear));
		m(2, 3) = 0;

		m(3, 0) = 0;
		m(3, 1) = 0;
		m(3, 2) = (T)(zNear / (zNear - zFar));
		m(3, 3) = 1;

		return m;
	}

	//! Builds a right-handed orthogonal projection matrix.
	template<typename T>
	FMatrix4<T> BuildProjectionMatrixOrtho(T left, T right, T bottom, T top, T zNear, T zFar)
	{
		T w = right - left;
		T h = top - bottom;

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = (T)(2 / w);
		m(0, 1) = 0;
		m(0, 2) = 0;
		m(0, 3) = 0;

		m(1, 0) = 0;
		m(1, 1) = (T)(2 / h);
		m(1, 2) = 0;
		m(1, 3) = 0;

		m(2, 0) = 0;
		m(2, 1) = 0;
		m(2, 2) = (T)(1 / (zFar - zNear));
		m(2, 3) = 0;

		m(3, 0) = -(left + right) / w;
		m(3, 1) = -(bottom + top) / h;
		m(3, 2) = (T)(zNear / (zNear - zFar));
		m(3, 3) = 1;

		return m;
	}

	//! Builds a right-handed look-at matrix.
	template <typename T>
	FMatrix4<T> BuildCameraLookAtMatrix(
		const FVec3<T>& position,
		const FVec3<T>& target,
		const FVec3<T>& upVector)
	{
		FVec3<T> zaxis = target - position;
		zaxis.Normalize();

		// Keep the same look at matrix with UE4
		//FVec3<T> xaxis = upVector.Cross(zaxis);
		FVec3<T> xaxis = zaxis.Cross(upVector);
		xaxis.Normalize();

		//FVec3<T> yaxis = zaxis.Cross(xaxis);
		FVec3<T> yaxis = xaxis.Cross(zaxis);
		yaxis.Normalize();

		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		m(0, 0) = (T)xaxis.X;
		m(0, 1) = (T)yaxis.X;
		m(0, 2) = (T)zaxis.X;
		m(0, 3) = 0;

		m(1, 0) = (T)xaxis.Y;
		m(1, 1) = (T)yaxis.Y;
		m(1, 2) = (T)zaxis.Y;
		m(1, 3) = 0;

		m(2, 0) = (T)xaxis.Z;
		m(2, 1) = (T)yaxis.Z;
		m(2, 2) = (T)zaxis.Z;
		m(2, 3) = 0;

		m(3, 0) = (T)-xaxis.Dot(position);
		m(3, 1) = (T)-yaxis.Dot(position);
		m(3, 2) = (T)-zaxis.Dot(position);
		m(3, 3) = 1;

		return m;
	}

	////! Builds a matrix that flattens geometry into a plane.
	///** \param light light source
	//	\param plane: plane into which the geometry if flattened into
	//	\param point: value between 0 and 1, describing the light source.
	//	If this is 1, it is a point light, if it is 0, it is a directional light. */
	template<typename T>
	FMatrix4<T> BuildShadowMatrix(const FVec3<T>& light, FPlane3D<T> plane, T point = 1.0f)
	{
		FMatrix4<T> m(FMatrix4<T>::EM4CONST_NOTHING);

		plane.Normal.Normalize();
		const T d = plane.Normal.Dot(light);

		m(0, 0) = (T)(-plane.Normal.X * light.X + d);
		m(0, 1) = (T)(-plane.Normal.X * light.Y);
		m(0, 2) = (T)(-plane.Normal.X * light.Z);
		m(0, 3) = (T)(-plane.Normal.X * point);

		m(1, 0) = (T)(-plane.Normal.Y * light.X);
		m(1, 1) = (T)(-plane.Normal.Y * light.Y + d);
		m(1, 2) = (T)(-plane.Normal.Y * light.Z);
		m(1, 3) = (T)(-plane.Normal.Y * point);

		m(2, 0) = (T)(-plane.Normal.Z * light.X);
		m(2, 1) = (T)(-plane.Normal.Z * light.Y);
		m(2, 2) = (T)(-plane.Normal.Z * light.Z + d);
		m(2, 3) = (T)(-plane.Normal.Z * point);

		m(3, 0) = (T)(-plane.D * light.X);
		m(3, 1) = (T)(-plane.D * light.Y);
		m(3, 2) = (T)(-plane.D * light.Z);
		m(3, 3) = (T)(-plane.D * point + d);

		return m;
	}
}

