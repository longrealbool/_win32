#if !defined(GAME_MATH_H)


union v2 {
  struct {
    
  real32 X, Y;
  };
  
  real32 E[2];
};

union v3 {
  struct {
    
    real32 X, Y, Z;
  };
  struct {
    
    real32 R, G, B;
  };
  struct {
    // NOTE(Egor):the best
    v2 XY;
    real32 Ignored0_;
  };
  real32 E[3];
};

union v4 {
  struct {
    
    real32 X, Y, Z, W;
  };
  struct {
    
    real32 R, G, B, A;
  };
  
  real32 E[4];
};


inline v2
V2(real32 X, real32 Y) {
  
  v2 Result;
  Result.X = X;
  Result.Y = Y;
  
  return Result;
}

inline v3
V3(real32 X, real32 Y, real32 Z) {
  
  v3 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  
  return Result;
}

inline v3
V3(v2 XY, real32 Z) {
  
  v3 Result;
  Result.X = XY.X;
  Result.Y = XY.Y;
  Result.Z = Z;
  
  return Result;
}

inline v4
V4(real32 X, real32 Y, real32 Z, real32 W) {
  
  v4 Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;
  
  return Result;
}

struct rectangle2 {
  
  v2 Min;
  v2 Max;
};

struct rectangle3 {
  
  v3 Min;
  v3 Max;
};


///
///////////////////////////////////
///

inline v2
operator*(real32 lambda, v2 B) {
  
  v2 Result;
  
  Result.X = B.X * lambda;
  Result.Y = B.Y * lambda;
  
  return Result;
  
}


inline v2
operator*(v2 B, real32 lambda) {
  
  v2 Result = lambda * B;
  
  return Result;
  
}

inline v2 &
operator*=(v2 &B, real32 A) {
  
  B =  A * B;
  return B;
  
}


inline v2
operator-(v2 A) {
  
  v2 Result;
  Result.X = -A.X;
  Result.Y = -A.Y;
  
  return Result;
}

inline v2
operator-(v2 A, v2 B) {
  
  v2 Result;
  Result.X = A.X - B.X; 
  Result.Y = A.Y - B.Y;
  
  return Result;
}

inline v2
operator+(v2 A, v2 B) {
  
  v2 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  
  return Result;
}

inline v2 &
operator +=(v2 &A, v2 B) {
  
  A = A + B;
  return A;
}

inline v2 &
operator -=(v2 &A, v2 B) {
  
  A = A - B;
  return A;
}

inline real32
Inner(v2 A, v2 B) {
  
  real32 Result = A.X * B.X + A.Y * B.Y;
  return Result;
}

inline real32
LengthSq(v2 A) {
  
  real32 Result = Inner(A, A);
  return Result;
}

inline real32
Length(v2 A) {
  
  real32 Result = SquareRoot(LengthSq(A));
  return Result;
}

///
////////////////////////////////////////////////
///

inline v3
operator*(real32 lambda, v3 B) {
  
  v3 Result;
  
  Result.X = B.X * lambda;
  Result.Y = B.Y * lambda;
  Result.Z = B.Z * lambda;
  
  return Result;
}


inline v3
operator*(v3 B, real32 lambda) {
  
  v3 Result = lambda * B;
  return Result;
}

inline v3 &
operator*=(v3 &B, real32 A) {
  
  B =  A * B;
  return B;
}


inline v3
operator-(v3 A) {
  
  v3 Result;
  Result.X = -A.X;
  Result.Y = -A.Y;
  Result.Z = -A.Z;
  
  return Result;
}

inline v3
operator-(v3 A, v3 B) {
  
  v3 Result;
  Result.X = A.X - B.X; 
  Result.Y = A.Y - B.Y;
  Result.Z = A.Z - B.Z;
  
  return Result;
}

inline v3
operator+(v3 A, v3 B) {
  
  v3 Result;
  Result.X = A.X + B.X;
  Result.Y = A.Y + B.Y;
  Result.Z = A.Z + B.Z;
  
  return Result;
}

inline v3 &
operator +=(v3 &A, v3 B) {
  
  A = A + B;
  return A;
}

inline v3 &
operator -=(v3 &A, v3 B) {
  
  A = A - B;
  return A;
}

inline real32
Inner(v3 A, v3 B) {
  
  real32 Result = A.X * B.X + A.Y * B.Y + A.Z * B.Z;
  return Result;
}

inline v3
Hadamard(v3 A, v3 B) {
  
  v3 Result = {A.X * B.X, A.Y * B.Y, A.Z * B.Z};
  
  return Result;
}

inline real32
LengthSq(v3 A) {
  
  real32 Result;
  Result = Inner(A, A);
  return Result;
}

inline real32
Length(v3 A) {
  
  real32 Result;
  Result = SquareRoot(LengthSq(A));
  return Result;
}

///
////////////////////////////////////////////////
///



inline real32
Square(real32 A) {
  
  real32 Result = A*A;
  return Result;
  
}



inline rectangle2 
RectMinMax(v2 Min, v2 Max) {
  
  rectangle2 Result;
  Result.Min = Min;
  Result.Max = Max;
  
  return Result;
}

inline rectangle2
RectCenterHalfDim(v2 Center, v2 HalfDim) {
  
  rectangle2 Result;
  Result.Min = Center - HalfDim;
  Result.Max = Center + HalfDim;
  
  return Result;
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim) {
  
  rectangle2 Result = RectCenterHalfDim(Center, 0.5f*Dim);
  
  return Result;
}


inline rectangle2
RectMinDim(v2 Min, v2 Dim) {
  
  rectangle2 Result;
  Result.Min = Min;
  Result.Max = Min + Dim;
  
  return Result;
}


inline bool32
IsInRectangle(rectangle2 Rect, v2 Test) {
  
  bool32 Result = ((Test.X >= Rect.Min.X) &&
                   (Test.Y >= Rect.Min.Y) &&
                   (Test.X < Rect.Max.X) &&
                   (Test.Y < Rect.Max.Y));
  
  return Result;
}

inline rectangle2 
AddRadiusTo(rectangle2 Source, v2 Radius) {
  
  rectangle2 Result;
  Result.Min = Source.Min - Radius;
  Result.Max = Source.Max + Radius;
  
  return Result;
}

inline v2
GetMinCorner(rectangle2 Rect) {
  
  v2 Result = Rect.Min;
  return Result;
}

inline v2
GetMaxCorner(rectangle2 Rect) {
  
  v2 Result = Rect.Max;
  return Result;
}

inline v2
GetCenter(rectangle2 Rect) {
  
  v2 Result = 0.5f*(Rect.Min + Rect.Max);
  return Result;
}

///
////////////////////////
///



inline rectangle3 
RectMinMax(v3 Min, v3 Max) {
  
  rectangle3 Result;
  Result.Min = Min;
  Result.Max = Max;
  
  return Result;
}

inline rectangle3
RectCenterHalfDim(v3 Center, v3 HalfDim) {
  
  rectangle3 Result;
  Result.Min = Center - HalfDim;
  Result.Max = Center + HalfDim;
  
  return Result;
}

inline rectangle3
RectCenterDim(v3 Center, v3 Dim) {
  
  rectangle3 Result = RectCenterHalfDim(Center, 0.5f*Dim);
  
  return Result;
}


inline rectangle3
RectMinDim(v3 Min, v3 Dim) {
  
  rectangle3 Result;
  Result.Min = Min;
  Result.Max = Min + Dim;
  
  return Result;
}


inline bool32
IsInRectangle(rectangle3 Rect, v3 Test) {
  
  bool32 Result = ((Test.X >= Rect.Min.X) &&
                   (Test.Y >= Rect.Min.Y) &&
                   (Test.Z >= Rect.Min.Z) &&
                   (Test.X < Rect.Max.X) &&
                   (Test.Y < Rect.Max.Y) &&
                   (Test.Z < Rect.Max.Z));
  
  return Result;
}

inline rectangle3
AddRadiusTo(rectangle3 Source, v3 Radius) {
  
  rectangle3 Result;
  Result.Min = Source.Min - Radius;
  Result.Max = Source.Max + Radius;
  
  return Result;
}

inline v3
GetMinCorner(rectangle3 Rect) {
  
  v3 Result = Rect.Min;
  return Result;
}

inline v3
GetMaxCorner(rectangle3 Rect) {
  
  v3 Result = Rect.Max;
  return Result;
}

inline v3
GetCenter(rectangle3 Rect) {
  
  v3 Result = 0.5f*(Rect.Min + Rect.Max);
  return Result;
}

#define GAME_MATH_H
#endif