#pragma once

#include "common/macros.hpp"
#include "common/mathUtilites.hpp"

#include "shiva/functions/bases/LagrangeBasis.hpp"
#include "shiva/functions/quadrature/Quadrature.hpp"
#include "shiva/functions/spacing/Spacing.hpp"
#include "shiva/geometry/shapes/NCube.hpp"
#include "shiva/geometry/shapes/InterpolatedShape.hpp"
#include "shiva/geometry/mapping/LinearTransform.hpp"
#include "shiva/common/ShivaMacros.hpp"
#include "shiva/common/pmpl.hpp"
#include "shiva/common/types.hpp"
#include "shiva/discretizations/finiteElementMethod/parentElements/ParentElement.hpp"

#include <stdio.h>

using namespace shiva;
using namespace shiva::functions;
using namespace shiva::geometry;
using namespace shiva::geometry::utilities;
using namespace shiva::discretizations::finiteElementMethod;

/**
 * This class is the basis class for the hexahedron finite element cells with shape functions defined on Gauss-Lobatto quadrature points.
 */
template< int ORDER,
          typename TRANSFORM,
          typename PARENT_ELEMENT >
class SEMQkGLIntegralsShiva
{
public:
  constexpr static bool isClassic = false;

  static constexpr int order = ORDER;
  static constexpr int numSupportPoints1d = ORDER + 1;

  using TransformType = TRANSFORM;
  using ParentElementType = PARENT_ELEMENT;

  using tfloat = typename TransformType::RealType;
  using gfloat = typename ParentElementType::RealType;

  using JacobianType = typename std::remove_reference_t< TransformType >::JacobianType;
  using quadrature = QuadratureGaussLobatto< gfloat, numSupportPoints1d >;
  using basisFunction = LagrangeBasis< gfloat, ORDER, GaussLobattoSpacing >;

  struct PrecomputedData
  {};

  PROXY_HOST_DEVICE
  static void init( PrecomputedData & )
  {}

  template< int qa, int qb, int qc, typename FUNC >
  static constexpr inline
  SEMKERNELS_HOST_DEVICE
  void computeGradPhiBGradPhi( tfloat const (&B)[6],
                               FUNC && func )
  {
    constexpr gfloat qcoords[3] = { quadrature::template coordinate< qa >(),
                                    quadrature::template coordinate< qb >(),
                                    quadrature::template coordinate< qc >() };
    constexpr gfloat w = quadrature::template weight< qa >() * quadrature::template weight< qb >() * quadrature::template weight< qc >();
    forSequence< numSupportPoints1d >( [&] ( auto const ici )
    {
      constexpr int i = decltype(ici)::value;      
      constexpr int ibc = linearIndex<ORDER>( i, qb, qc );
      constexpr int aic = linearIndex<ORDER>( qa, i, qc );
      constexpr int abi = linearIndex<ORDER>( qa, qb, i );
      constexpr gfloat gia = basisFunction::template gradient< i >( qcoords[0] );
      constexpr gfloat gib = basisFunction::template gradient< i >( qcoords[1] );
      constexpr gfloat gic = basisFunction::template gradient< i >( qcoords[2] );
//      printf("i: %d, ibc: %d, aic: %d, abi: %d, gia: %f, gib: %f, gic: %f\n", i, ibc, aic, abi, gia, gib, gic);

      forSequence< numSupportPoints1d >( [&] ( auto const icj )
      {
        constexpr int j = decltype(icj)::value;
        constexpr int jbc = linearIndex<ORDER>( j, qb, qc );
        constexpr int ajc = linearIndex<ORDER>( qa, j, qc );
        constexpr int abj = linearIndex<ORDER>( qa, qb, j );
        constexpr gfloat gja = basisFunction::template gradient< j >( qcoords[0] );
        constexpr gfloat gjb = basisFunction::template gradient< j >( qcoords[1] );
        constexpr gfloat gjc = basisFunction::template gradient< j >( qcoords[2] );

//        printf("j: %d, jbc: %d, ajc: %d, abj: %d, gja: %f, gjb: %f, gjc: %f\n", j, jbc, ajc, abj, gja, gjb, gjc);
        // diagonal terms
        constexpr gfloat w0 = w * gia * gja;
        func( ibc, jbc, w0 * B[0] );
        constexpr gfloat w1 = w * gib * gjb;
        func( aic, ajc, w1 * B[1] );
        constexpr gfloat w2 = w * gic * gjc;
        func( abi, abj, w2 * B[2] );
        // off-diagonal terms
        // const gfloat w3 = w * gib * gjc;
        // func( aic, abj, w3 * B[3] );
        // func( abj, aic, w3 * B[3] );
        // const gfloat w4 = w * gia * gjc;
        // func( ibc, abj, w4 * B[4] );
        // func( abj, ibc, w4 * B[4] );
        // const gfloat w5 = w * gia * gjb;
        // func( ibc, ajc, w5 * B[5] );
        // func( ajc, ibc, w5 * B[5] );
      } );
    } );
  }



  template< typename FUNC >
  static constexpr inline
  SEMKERNELS_HOST_DEVICE
  void computeStiffnessAndMassTerm( TransformType const & trilinearCell,
                             float massMatrix[],
                             FUNC && func )
  {

    // this is a compile time quadrature loop over each tensor direction
    forNestedSequence< ORDER + 1,
                       ORDER + 1,
                       ORDER + 1 >( [&] ( auto const icqa,
                                          auto const icqb,
                                          auto const icqc )
    {
      constexpr int qa = decltype(icqa)::value;
      constexpr int qb = decltype(icqb)::value;
      constexpr int qc = decltype(icqc)::value;

      JacobianType J{ tfloat(0.0) };

      shiva::geometry::utilities::jacobian< quadrature, qa, qb, qc >( trilinearCell, J );

      tfloat const detJ = determinant( J );
      
      // mass matrix
      constexpr int q = linearIndex<ORDER>( qc, qb, qa );
      constexpr tfloat w3D = quadrature::template weight< qa >() *
                             quadrature::template weight< qb >() *
                             quadrature::template weight< qc >();
      massMatrix[q] = w3D * detJ;

      tfloat B[6] = {0};
      computeB( J, B );

//      printf( "B(%d,%d,%d) = | %18.14e %18.14e %18.14e %18.14e %18.14e %18.14e |\n", qa, qb, qc, B[0], B[1], B[2], B[3], B[4], B[5] );

      // compute detJ*J^{-1}J^{-T}
      for( int i = 0; i < 6; ++i )
      {
       B[i] *= detJ;
      }

      // compute gradPhiI*B*gradPhiJ and stiffness vector
      computeGradPhiBGradPhi< qa, qb, qc >( B, func );
    } );
  }


  // template< typename ARRAY_REAL_VIEW, typename LOCAL_ARRAY >
  template< typename LOCAL_ARRAY >
  static constexpr inline
  SEMKERNELS_HOST_DEVICE
  void
  gatherCoordinates( const int & elementNumber,
                     const float X[8][3],
                     LOCAL_ARRAY & cellData )
  {
    int corner_id = 0;
    for ( int k = 0; k < 2; ++k )
    {
      for ( int j = 0; j < 2; ++j )
      {
        for ( int i = 0; i < 2; ++i )
        {
          int const l = linearIndex<1>( i, j, k );
          cellData( i, j, k, 0 ) = X[corner_id][0];
          cellData( i, j, k, 1 ) = X[corner_id][1];
          cellData( i, j, k, 2 ) = X[corner_id][2];
          corner_id++;
        }
      }
    }
  }

  /**
   * @brief compute  mass Matrix stiffnessVector.
   */
  // template< typename ARRAY_REAL_VIEW >
  static constexpr inline
  SEMKERNELS_HOST_DEVICE
  void computeMassMatrixAndStiffnessVector( const int & elementNumber,
                                            const int & nPointsPerElement,
                                            const float X[8][3],
                                            PrecomputedData const & precomputedData,
                                            float massMatrixLocal[],
                                            float pnLocal[],
                                            float Y[] )
  {
    TransformType trilinearCell;
    typename TransformType::DataType & cellCoordData = trilinearCell.getData();

    gatherCoordinates( elementNumber,
                       X,
                       cellCoordData );

    computeStiffnessAndMassTerm( trilinearCell, massMatrixLocal, [&] ( const int i, const int j, const auto val )
    {
      Y[i] = Y[i] + val * pnLocal[j];
    } );
  }

};
