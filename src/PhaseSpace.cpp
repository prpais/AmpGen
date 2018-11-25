#include "AmpGen/PhaseSpace.h"

#include <RtypesCore.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include "AmpGen/ParticleProperties.h"
#include "AmpGen/ParticlePropertiesList.h"
#include "AmpGen/Types.h"

#include "TRandom.h"
const Int_t kMAXP = 18;

using namespace AmpGen;

double PhaseSpace::PDK( double a, double b, double c )
{
  double x = ( a - b - c ) * ( a + b + c ) * ( a - b + c ) * ( a + b - c );
  x        = sqrt( x ) / ( 2 * a );
  return x;
}

Event PhaseSpace::makeEvent(const size_t& cacheSize)
{
  std::array<double, kMAXP> rno;
  std::array<double, kMAXP> pd;
  std::array<double, kMAXP> invMas; 
  Event rt( 4 * m_nt, cacheSize);

  rno[0] = 0;
  unsigned int n;

  double wt = m_wtMax;
  do {
    wt     = m_wtMax;
    rno[0] = 0;
    for ( n = 1; n < m_nt - 1; n++ ) rno[n] = rndm(); // m_nt-2 random numbers
    rno[m_nt - 1]                           = 1;
    std::sort( rno.begin() + 1, rno.begin() + m_nt );

    double sum = 0;
    for ( n = 0; n < m_nt; n++ ) {
      sum += m_mass[n];
      invMas[n] = rno[n] * m_teCmTm + sum;
    }
    for ( n = 0; n < m_nt - 1; n++ ) {
      pd[n] = PDK( invMas[n + 1], invMas[n], m_mass[n + 1] );
      wt *= pd[n];
    }
  } while ( wt < rndm() );
  
  rt.set(0, { 0, pd[0], 0, sqrt( pd[0] * pd[0] + m_mass[0] * m_mass[0] )} );

  for( unsigned int i = 1 ; i != m_nt ; ++i ){  
    rt.set( i, { 0, -pd[i - 1], 0, sqrt( pd[i - 1] * pd[i - 1] + m_mass[i] * m_mass[i] ) } );
    double cZ   = 2 * rndm() - 1;
    double sZ   = sqrt( 1 - cZ * cZ );
    double angY = 2 * M_PI * rndm();
    double cY   = cos( angY );
    double sY   = sin( angY );
    for ( unsigned int j = 0; j <= i; j++ ) {
      double x          = rt[4*j+0];
      double y          = rt[4*j+1];
      double z          = rt[4*j+2];
      rt[4*j+0] = cZ * x - sZ * y;
      rt[4*j+1] = sZ * x + cZ * y;
      x         = rt[4*j+0];
      rt[4*j+0] = cY * x - sY * z;
      rt[4*j+2] = sY * x + cY * z;
    }
    if ( i == ( m_nt - 1 ) ) break;
    double beta = pd[i] / sqrt( pd[i] * pd[i] + invMas[i] * invMas[i] );
    double gamma = 1./sqrt( 1 -beta*beta);
    for ( unsigned int j = 0; j <= i; j++ ){
      double E = rt[4*j+3];
      double py = rt[4*j+1];
      rt[4*j+1] = gamma*( py + beta * E );
      rt[4*j+3] = gamma*( E + beta * py );
    }
  }
  rt.setGenPdf( 1 );
  if ( m_type.isTimeDependent() ) rt.set( 4 * m_nt, m_rand->Exp( m_decayTime ) );
  return rt;
}

Bool_t PhaseSpace::setDecay( const double& m0, const std::vector<double>& mass )
{
  unsigned int n;
  m_nt = mass.size();

  m_teCmTm = m0;
  for ( n = 0; n < m_nt; n++ ) {
    m_mass[n] = mass[n];
    m_teCmTm -= mass[n];
  }

  if ( m_teCmTm <= 0 ) return 0 ; 

  double emmax = m_teCmTm + m_mass[0];
  double emmin = 0;
  double wtmax = 1;
  for ( n = 1; n < m_nt; n++ ) {
    emmin += m_mass[n - 1];
    emmax += m_mass[n];
    wtmax *= PDK( emmax, emmin, m_mass[n] );
  }
  m_wtMax = 1 / wtmax;

  return true;
}


PhaseSpace::PhaseSpace( const EventType& type, TRandom* rand ) : m_rand( rand ), m_type(type) 
{
  setDecay( type.motherMass(), type.masses() );
  if ( type.isTimeDependent() )
    m_decayTime = 6.582119514 / ( ParticlePropertiesList::get( type.mother() )->width() * pow( 10, 13 ) );
}

AmpGen::EventType PhaseSpace::eventType() const { return m_type; }

