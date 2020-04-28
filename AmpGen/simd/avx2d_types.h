#ifndef AMPGEN_AVXd_TYPES
#define AMPGEN_AVXd_TYPES 1

#include <immintrin.h>
#include <array>
#include <iostream>
#include <complex>
#include <omp.h>

namespace AmpGen {
  namespace AVX2d {
    #define stl_fallback( x ) \
      inline real_v x( const real_v& v ){  auto a = v.to_array(); return real_v( std::x(a[0]), std::x(a[1]), std::x(a[2]), std::x(a[3]) ) ; } 
       
    struct real_v {
      __m256d data;
      static constexpr unsigned size = 4;
      typedef double scalar_type;
      real_v() = default; 
      real_v(__m256d data ) : data(data) {}
      real_v(const double& f ) : data( _mm256_set1_pd( f )) {}
      real_v(const double& x0, const double& x1, const double& x2, const double& x3 )
      {
        double tmp[4] = {x0,x1,x2,x3};
        data = _mm256_loadu_pd(tmp); 
      }
      real_v(const double* f ) : data( _mm256_loadu_pd( f ) ) {}
      void store( double* ptr ) const { _mm256_storeu_pd( ptr, data ); }
      std::array<double, 4> to_array() const { std::array<double, 4> b; store( &b[0] ); return b; }
      double at(const unsigned i) const { return to_array()[i] ; }       
      operator __m256d() const { return data ; } 
    };

    inline real_v operator+( const real_v& lhs, const real_v& rhs ) { return _mm256_add_pd(lhs, rhs); }
    inline real_v operator-( const real_v& lhs, const real_v& rhs ) { return _mm256_sub_pd(lhs, rhs); }
    inline real_v operator*( const real_v& lhs, const real_v& rhs ) { return _mm256_mul_pd(lhs, rhs); }
    inline real_v operator/( const real_v& lhs, const real_v& rhs ) { return _mm256_div_pd(lhs, rhs); }
    inline real_v operator-( const real_v& x ) { return -1.f * x; }
    inline real_v operator&( const real_v& lhs, const real_v& rhs ) { return _mm256_and_pd( lhs, rhs ); }
    inline real_v operator|( const real_v& lhs, const real_v& rhs ) { return _mm256_or_pd( lhs, rhs ); }
    inline real_v operator^( const real_v& lhs, const real_v& rhs ) { return _mm256_xor_pd( lhs, rhs ); }
    inline real_v operator+=(real_v& lhs, const real_v& rhs ){ lhs = lhs + rhs; return lhs; }
    inline real_v operator-=(real_v& lhs, const real_v& rhs ){ lhs = lhs - rhs; return lhs; }
    inline real_v operator*=(real_v& lhs, const real_v& rhs ){ lhs = lhs * rhs; return lhs; }
    inline real_v operator/=(real_v& lhs, const real_v& rhs ){ lhs = lhs / rhs; return lhs; }
    inline real_v operator&&( const real_v& lhs, const real_v& rhs ) { return _mm256_and_pd( lhs, rhs ); }
    inline real_v operator||( const real_v& lhs, const real_v& rhs ) { return _mm256_or_pd( lhs, rhs ); }
    inline real_v operator!( const real_v& x ) { return x ^ _mm256_castsi256_pd( _mm256_set1_epi32( -1 ) ); }
    inline real_v operator<( const real_v& lhs, const real_v& rhs ) { return _mm256_cmp_pd( lhs, rhs, _CMP_LT_OS ); }
    inline real_v operator>( const real_v& lhs, const real_v& rhs ) { return _mm256_cmp_pd( lhs, rhs, _CMP_GT_OS ); }
    inline real_v operator==( const real_v& lhs, const real_v& rhs ){ return _mm256_cmp_pd( lhs, rhs, _CMP_EQ_OS ); }
    inline real_v sqrt( const real_v& v ) { return _mm256_sqrt_pd(v); } 
    inline real_v abs ( const real_v& v ) { return _mm256_andnot_pd(_mm256_set1_pd(-0.), v);  }
    // inline real_v sin( const real_v& v )  { return sin256_pd(v) ; }
    // inline real_v cos( const real_v& v )  { return cos256_pd(v) ; }
    // inline real_v tan( const real_v& v )  { real_v s; real_v c; sincos256_pd(v, (__m256*)&s, (__m256*)&c) ; return s/c; }
    // inline real_v exp( const real_v& v )  { return exp256_ps(v) ; }
    inline real_v select(const real_v& mask, const real_v& a, const real_v& b ) { return _mm256_blendv_pd( b, a, mask ); }
    inline real_v select(const bool& mask   , const real_v& a, const real_v& b ) { return mask ? a : b; } 
    inline real_v sign  ( const real_v& v){ return select( v > 0., +1., -1. ); }
    inline real_v atan2( const real_v& y, const real_v& x ){ 
      std::array<double, 4> bx{x.to_array()}, by{y.to_array()};
      return real_v ( 
          std::atan2(   by[0], bx[0]) 
          , std::atan2( by[1], bx[1]) 
          , std::atan2( by[2], bx[2]) 
          , std::atan2( by[3], bx[3]) ); 
    }
    inline __m256i udouble_to_uint( const real_v& x )
    {
      auto xr = _mm256_round_pd(x, _MM_FROUND_TO_NEG_INF);
      // based on:  https://stackoverflow.com/questions/41144668/how-to-efficiently-perform-double-int64-conversions-with-sse-avx
      return _mm256_sub_epi64(_mm256_castpd_si256(_mm256_add_pd(xr, _mm256_set1_pd(0x0018000000000000))), 
             _mm256_castpd_si256(_mm256_set1_pd(0x0018000000000000)));
    }
    inline real_v gather( const double* base_addr, const real_v& offsets) 
    {
     return _mm256_i64gather_pd(base_addr, udouble_to_uint(offsets),sizeof(double));
    } 
    
    inline void frexp(const real_v& value, real_v& mant, real_v& exponent)
    {
      auto arg_as_int = _mm256_castpd_si256(value);
      static const real_v offset(4503599627370496.0 + 1022.0);   // 2^52 + 1022.0
      static const __m256i pow2_52_i = _mm256_set1_epi64x(0x4330000000000000); // *reinterpret_cast<const uint64_t*>(&pow2_52_d);
      auto b = _mm256_srl_epi64(arg_as_int, _mm_cvtsi32_si128(52));
      auto c = _mm256_or_si256( b , pow2_52_i);
      exponent = real_v( _mm256_castsi256_pd(c) ) - offset;
      mant     = _mm256_castsi256_pd(_mm256_or_si256(_mm256_and_si256 (arg_as_int, _mm256_set1_epi64x(0x000FFFFFFFFFFFFFll) ), _mm256_set1_epi64x(0x3FE0000000000000ll)));
    }
    
    inline real_v fmadd( const real_v& a, const real_v& b, const real_v& c )
    {
      return _mm256_fmadd_pd(a, b, c);
    }
    inline real_v log(const real_v& arg)
    {
      static const real_v corr     = 0.693147180559945286226764;
      static const real_v CL15     = 0.148197055177935105296783;
      static const real_v CL13     = 0.153108178020442575739679;
      static const real_v CL11     = 0.181837339521549679055568;
      static const real_v CL9      = 0.22222194152736701733275;
      static const real_v CL7      = 0.285714288030134544449368;
      static const real_v CL5      = 0.399999999989941956712869;
      static const real_v CL3      = 0.666666666666685503450651;
      static const real_v CL1      = 2.0;
      real_v mant, exponent;
      frexp(arg, mant, exponent);
      auto x  = (mant - 1.) / (mant + 1.);
      auto x2 = x * x;
      auto p = fmadd(CL15, x2, CL13);
      p = fmadd(p, x2, CL11);
      p = fmadd(p, x2, CL9);
      p = fmadd(p, x2, CL7);
      p = fmadd(p, x2, CL5);
      p = fmadd(p, x2, CL3);
      p = fmadd(p, x2, CL1);
      p = fmadd(p, x, corr * exponent);
      return p;
    }
    stl_fallback( exp )
    stl_fallback( tan )
    stl_fallback( sin )
    stl_fallback( cos )
    inline real_v remainder( const real_v& a, const real_v& b ){ return a - real_v(_mm256_round_pd(a/b, _MM_FROUND_TO_NEG_INF)) * b; }
    inline real_v fmod( const real_v& a, const real_v& b )
    {
      auto r = remainder( abs(a), abs(b) );
      return select( a > 0., r, -r );
    }

    struct complex_v {
      real_v re;
      real_v im;
      typedef std::complex<double> scalar_type;
      static constexpr unsigned size = 4;

      real_v real() const { return re; }
      real_v imag() const { return im; }
      real_v norm() const { return re*re + im *im ; }
      complex_v() = default; 
      complex_v( const real_v& re, const real_v& im) : re(re), im(im) {}
      complex_v( const float&   re, const float& im) : re(re), im(im) {}
      complex_v( const std::complex<double>& f ) : re( f.real() ), im( f.imag() ) {}
      complex_v( const std::complex<float>& f  ) : re( f.real() ), im( f.imag() ) {}
      explicit complex_v( const real_v& arg ) : re(arg) {};
      explicit complex_v( const double& arg ) : re(arg) {};
      const std::complex<double> at(const unsigned i) const { return std::complex<float>(re.to_array()[i], im.to_array()[i]) ; }       
      void store( double* sre, double* sim ){ re.store(sre); im.store(sim);  } 
      void store( scalar_type* r ) const { 
        auto re_arr = re.to_array();
        auto im_arr = im.to_array();
        for( unsigned i = 0 ; i != re_arr.size(); ++i ) r[i] = scalar_type( re_arr[i], im_arr[i] ); 
      }
      auto to_array() const 
      {
        std::array<scalar_type, 4> rt;
        store( rt.data() );
        return rt;
      }
    };
   
    inline std::ostream& operator<<( std::ostream& os, const real_v& obj ) { 
      auto buffer = obj.to_array();
      for( unsigned i = 0 ; i != 4; ++i ) os << buffer[i] << " ";
      return os; 
    }
    inline real_v     real(const complex_v& arg ){ return arg.re ; }
    inline real_v     imag(const complex_v& arg ){ return arg.im ; }
    inline complex_v   conj(const complex_v& arg ){ return complex_v(arg.re, -arg.im) ; }
    inline real_v     conj(const real_v& arg ){ return arg ; } 
    inline complex_v operator+( const complex_v& lhs, const real_v& rhs ) { return complex_v(lhs.re + rhs, lhs.im); }
    inline complex_v operator-( const complex_v& lhs, const real_v& rhs ) { return complex_v(lhs.re - rhs, lhs.im); }
    inline complex_v operator*( const complex_v& lhs, const real_v& rhs ) { return complex_v(lhs.re*rhs, lhs.im*rhs); }
    inline complex_v operator/( const complex_v& lhs, const real_v& rhs ) { return complex_v(lhs.re/rhs, lhs.im/rhs); }
    inline complex_v operator+( const real_v& lhs, const complex_v& rhs ) { return complex_v(lhs + rhs.re,  rhs.im); }
    inline complex_v operator-( const real_v& lhs, const complex_v& rhs ) { return complex_v(lhs - rhs.re, - rhs.im); }
    inline complex_v operator*( const real_v& lhs, const complex_v& rhs ) { return complex_v(lhs*rhs.re, lhs*rhs.im); }
    inline complex_v operator/( const real_v& lhs, const complex_v& rhs ) { return complex_v( lhs * rhs.re , -lhs *rhs.im) / (rhs.re * rhs.re + rhs.im * rhs.im ); }
    inline complex_v operator+( const complex_v& lhs, const complex_v& rhs ) { return complex_v(lhs.re + rhs.re, lhs.im + rhs.im); }
    inline complex_v operator-( const complex_v& lhs, const complex_v& rhs ) { return complex_v(lhs.re - rhs.re, lhs.im - rhs.im); }
    inline complex_v operator*( const complex_v& lhs, const complex_v& rhs ) { return complex_v(lhs.re*rhs.re - lhs.im*rhs.im, lhs.re*rhs.im  + lhs.im*rhs.re); }
    inline complex_v operator/( const complex_v& lhs, const complex_v& rhs ) { return complex_v(lhs.re*rhs.re + lhs.im*rhs.im, -lhs.re*rhs.im  + lhs.im*rhs.re) / (rhs.re * rhs.re + rhs.im * rhs.im ); }
    inline complex_v operator-( const complex_v& x ) { return -1.f * x; }
    inline real_v abs( const complex_v& v ) { return sqrt( v.re * v.re + v.im * v.im ) ; }
    inline real_v norm( const complex_v& v ) { return  ( v.re * v.re + v.im * v.im ) ; }
    inline complex_v select(const real_v& mask, const complex_v& a, const complex_v& b ) { return complex_v( select(mask, a.re, b.re), select(mask, a.im, b.im ) ) ; }
    inline complex_v select(const real_v& mask, const real_v&   a, const complex_v& b ) { return complex_v( select(mask, a   , b.re), select(mask, 0.f, b.im) ); }
    inline complex_v select(const real_v& mask, const complex_v& a, const real_v& b   ) { return complex_v( select(mask, a.re, b )  , select(mask, a.im, 0.f) ); }
    inline complex_v select(const bool& mask   , const complex_v& a, const complex_v& b ) { return mask ? a : b; }
    inline complex_v exp( const complex_v& v ){ 
      return exp( v.re) * complex_v( cos( v.im ), sin( v.im ) );
    }
    inline complex_v sqrt( const complex_v& v )
    {
      auto r = abs(v);
      return complex_v ( sqrt( 0.5 * (r + v.re) ), sign(v.im) * sqrt( 0.5*( r - v.re ) ) );
    }
    inline complex_v log( const complex_v& v )
    {
      return complex_v( log( v.re ) , atan2(v.im, v.re) );
    } 

    inline std::ostream& operator<<( std::ostream& os, const complex_v& obj ) { return os << "( "<< obj.re << ") (" << obj.im << ")"; }
    #pragma omp declare reduction(+: real_v: \
	omp_out = omp_out + omp_in) 
    #pragma omp declare reduction(+: complex_v: \
	omp_out = omp_out + omp_in) 
  
  }  
}

#endif