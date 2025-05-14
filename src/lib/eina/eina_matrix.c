/* EINA - Drawing Library
 * Copyright (C) 2007-2014 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eina_private.h"

#include <math.h>
#include <float.h>
#include <string.h>

#include "eina_fp.h"
#include "eina_rectangle.h"
#include "eina_quad.h"
#include "eina_matrix.h"
#include "eina_util.h"

#define MATRIX_XX(m) (m)->xx
#define MATRIX_XY(m) (m)->xy
#define MATRIX_XZ(m) (m)->xz
#define MATRIX_XW(m) (m)->xw
#define MATRIX_YX(m) (m)->yx
#define MATRIX_YY(m) (m)->yy
#define MATRIX_YZ(m) (m)->yz
#define MATRIX_YW(m) (m)->yw
#define MATRIX_ZX(m) (m)->zx
#define MATRIX_ZY(m) (m)->zy
#define MATRIX_ZZ(m) (m)->zz
#define MATRIX_ZW(m) (m)->zw
#define MATRIX_WX(m) (m)->wx
#define MATRIX_WY(m) (m)->wy
#define MATRIX_WZ(m) (m)->wz
#define MATRIX_WW(m) (m)->ww
#define MATRIX_SIZE 9

#define QUAD_X0(q) q->x0
#define QUAD_Y0(q) q->y0
#define QUAD_X1(q) q->x1
#define QUAD_Y1(q) q->y1
#define QUAD_X2(q) q->x2
#define QUAD_Y2(q) q->y2
#define QUAD_X3(q) q->x3
#define QUAD_Y3(q) q->y3

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @internal
 * @brief Retrieves the type of a 3x3 floating-point matrix.
 * This function analyzes the components of the matrix @p m to determine if it's
 * an identity, affine, or projective matrix.
 *
 * @param m The matrix to analyze. Must not be NULL.
 * @return The type of the matrix (EINA_MATRIX_TYPE_IDENTITY,
 *         EINA_MATRIX_TYPE_AFFINE, or EINA_MATRIX_TYPE_PROJECTIVE).
 */
EINA_API Eina_Matrix_Type
eina_matrix3_type_get(const Eina_Matrix3 *m)
{
   if (!EINA_DBL_EQ(MATRIX_ZX(m), 0.0) ||
       !EINA_DBL_EQ(MATRIX_ZY(m), 0.0) ||
       !EINA_DBL_EQ(MATRIX_ZZ(m), 1.0))
     return EINA_MATRIX_TYPE_PROJECTIVE;
   else
     {
        if (EINA_DBL_EQ(MATRIX_XX(m), 1.0) &&
            EINA_DBL_EQ(MATRIX_XY(m), 0.0) &&
            EINA_DBL_EQ(MATRIX_XZ(m), 0.0) &&
            EINA_DBL_EQ(MATRIX_YX(m), 0.0) &&
            EINA_DBL_EQ(MATRIX_YY(m), 1.0) &&
            EINA_DBL_EQ(MATRIX_YZ(m), 0.0))
          return EINA_MATRIX_TYPE_IDENTITY;
        else
          return EINA_MATRIX_TYPE_AFFINE;
     }
}

/**
 * @internal
 * @brief Retrieves the type of a 4x4 floating-point matrix.
 * This function checks if the matrix @p m is an identity matrix or a more
 * general affine matrix. Note that 4x4 matrices in this context are
 * typically used for 3D affine transformations, so it distinguishes
 * between identity and general affine.
 *
 * @param m The matrix to analyze. Must not be NULL.
 * @return EINA_MATRIX_TYPE_IDENTITY if it's an identity matrix,
 *         otherwise EINA_MATRIX_TYPE_AFFINE.
 */
EINA_API Eina_Matrix_Type
eina_matrix4_type_get(const Eina_Matrix4 *m)
{
   if (EINA_DBL_EQ(MATRIX_XX(m), 1.0) &&
       EINA_DBL_EQ(MATRIX_XY(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_XZ(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_XW(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_YX(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_YY(m), 1.0) &&
       EINA_DBL_EQ(MATRIX_YZ(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_YW(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_ZX(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_ZY(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_ZZ(m), 1.0) &&
       EINA_DBL_EQ(MATRIX_ZW(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_WX(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_WY(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_WZ(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_WW(m), 1.0))
     return EINA_MATRIX_TYPE_IDENTITY;
   return EINA_MATRIX_TYPE_AFFINE;
}

/**
 * @internal
 * @brief Retrieves the type of a 3x3 fixed-point (Eina_F16p16) matrix.
 * This function analyzes the components of the fixed-point matrix @p m
 * to determine if it's an identity, affine, or projective matrix.
 * The value 65536 represents 1.0 in Eina_F16p16 format.
 *
 * @param m The fixed-point matrix to analyze. Must not be NULL.
 * @return The type of the matrix (EINA_MATRIX_TYPE_IDENTITY,
 *         EINA_MATRIX_TYPE_AFFINE, or EINA_MATRIX_TYPE_PROJECTIVE).
 */
EINA_API Eina_Matrix_Type
eina_matrix3_f16p16_type_get(const Eina_Matrix3_F16p16 *m)
{
   if ((MATRIX_ZX(m) != 0) || (MATRIX_ZY(m) != 0) || (MATRIX_ZZ(m) != 65536))
     return EINA_MATRIX_TYPE_PROJECTIVE;
   else
     {
        if ((MATRIX_XX(m) == 65536) && (MATRIX_XY(m) == 0) && (MATRIX_XZ(m) == 0) &&
            (MATRIX_YX(m) == 0) && (MATRIX_YY(m) == 65536) && (MATRIX_YZ(m) == 0))
          return EINA_MATRIX_TYPE_IDENTITY;
        else
          return EINA_MATRIX_TYPE_AFFINE;
     }
}

/**
 * @internal
 * @brief Sets the individual component values of a 3x3 floating-point matrix.
 *
 * @param[out] m The matrix to modify. Must not be NULL.
 * @param[in] xx Value for the [0][0] component.
 * @param[in] xy Value for the [0][1] component.
 * @param[in] xz Value for the [0][2] component (translation X).
 * @param[in] yx Value for the [1][0] component.
 * @param[in] yy Value for the [1][1] component.
 * @param[in] yz Value for the [1][2] component (translation Y).
 * @param[in] zx Value for the [2][0] component (perspective X).
 * @param[in] zy Value for the [2][1] component (perspective Y).
 * @param[in] zz Value for the [2][2] component (perspective W).
 */
EINA_API void
eina_matrix3_values_set(Eina_Matrix3 *m,
                        double xx, double xy, double xz,
                        double yx, double yy, double yz,
                        double zx, double zy, double zz)
{
   MATRIX_XX(m) = xx;
   MATRIX_XY(m) = xy;
   MATRIX_XZ(m) = xz;
   MATRIX_YX(m) = yx;
   MATRIX_YY(m) = yy;
   MATRIX_YZ(m) = yz;
   MATRIX_ZX(m) = zx;
   MATRIX_ZY(m) = zy;
   MATRIX_ZZ(m) = zz;
}

/**
 * @internal
 * @brief Retrieves the individual component values of a 3x3 floating-point matrix.
 *
 * @param[in] m The matrix to read from. Must not be NULL.
 * @param[out] xx Pointer to store the [0][0] component. Can be NULL.
 * @param[out] xy Pointer to store the [0][1] component. Can be NULL.
 * @param[out] xz Pointer to store the [0][2] component. Can be NULL.
 * @param[out] yx Pointer to store the [1][0] component. Can be NULL.
 * @param[out] yy Pointer to store the [1][1] component. Can be NULL.
 * @param[out] yz Pointer to store the [1][2] component. Can be NULL.
 * @param[out] zx Pointer to store the [2][0] component. Can be NULL.
 * @param[out] zy Pointer to store the [2][1] component. Can be NULL.
 * @param[out] zz Pointer to store the [2][2] component. Can be NULL.
 */
EINA_API void
eina_matrix3_values_get(const Eina_Matrix3 *m,
                        double *xx, double *xy, double *xz,
                        double *yx, double *yy, double *yz,
                        double *zx, double *zy, double *zz)
{
   if (xx) *xx = MATRIX_XX(m);
   if (xy) *xy = MATRIX_XY(m);
   if (xz) *xz = MATRIX_XZ(m);
   if (yx) *yx = MATRIX_YX(m);
   if (yy) *yy = MATRIX_YY(m);
   if (yz) *yz = MATRIX_YZ(m);
   if (zx) *zx = MATRIX_ZX(m);
   if (zy) *zy = MATRIX_ZY(m);
   if (zz) *zz = MATRIX_ZZ(m);
}

/**
 * @internal
 * @brief Sets the individual component values of a 4x4 floating-point matrix.
 *
 * @param[out] m The matrix to modify. Must not be NULL.
 * @param[in] xx Value for the [0][0] component.
 * @param[in] xy Value for the [0][1] component.
 * @param[in] xz Value for the [0][2] component.
 * @param[in] xw Value for the [0][3] component (translation X).
 * @param[in] yx Value for the [1][0] component.
 * @param[in] yy Value for the [1][1] component.
 * @param[in] yz Value for the [1][2] component.
 * @param[in] yw Value for the [1][3] component (translation Y).
 * @param[in] zx Value for the [2][0] component.
 * @param[in] zy Value for the [2][1] component.
 * @param[in] zz Value for the [2][2] component.
 * @param[in] zw Value for the [2][3] component (translation Z).
 * @param[in] wx Value for the [3][0] component (perspective X).
 * @param[in] wy Value for the [3][1] component (perspective Y).
 * @param[in] wz Value for the [3][2] component (perspective Z).
 * @param[in] ww Value for the [3][3] component (perspective W).
 */
EINA_API void
eina_matrix4_values_set(Eina_Matrix4 *m,
                        double xx, double xy, double xz, double xw,
                        double yx, double yy, double yz, double yw,
                        double zx, double zy, double zz, double zw,
                        double wx, double wy, double wz, double ww)
{
   MATRIX_XX(m) = xx;
   MATRIX_XY(m) = xy;
   MATRIX_XZ(m) = xz;
   MATRIX_XW(m) = xw;
   MATRIX_YX(m) = yx;
   MATRIX_YY(m) = yy;
   MATRIX_YZ(m) = yz;
   MATRIX_YW(m) = yw;
   MATRIX_ZX(m) = zx;
   MATRIX_ZY(m) = zy;
   MATRIX_ZZ(m) = zz;
   MATRIX_ZW(m) = zw;
   MATRIX_WX(m) = wx;
   MATRIX_WY(m) = wy;
   MATRIX_WZ(m) = wz;
   MATRIX_WW(m) = ww;
}

/**
 * @internal
 * @brief Retrieves the individual component values of a 4x4 floating-point matrix.
 *
 * @param[in] m The matrix to read from. Must not be NULL.
 * @param[out] xx Pointer to store the [0][0] component. Can be NULL.
 * @param[out] xy Pointer to store the [0][1] component. Can be NULL.
 * @param[out] xz Pointer to store the [0][2] component. Can be NULL.
 * @param[out] xw Pointer to store the [0][3] component. Can be NULL.
 * @param[out] yx Pointer to store the [1][0] component. Can be NULL.
 * @param[out] yy Pointer to store the [1][1] component. Can be NULL.
 * @param[out] yz Pointer to store the [1][2] component. Can be NULL.
 * @param[out] yw Pointer to store the [1][3] component. Can be NULL.
 * @param[out] zx Pointer to store the [2][0] component. Can be NULL.
 * @param[out] zy Pointer to store the [2][1] component. Can be NULL.
 * @param[out] zz Pointer to store the [2][2] component. Can be NULL.
 * @param[out] zw Pointer to store the [2][3] component. Can be NULL.
 * @param[out] wx Pointer to store the [3][0] component. Can be NULL.
 * @param[out] wy Pointer to store the [3][1] component. Can be NULL.
 * @param[out] wz Pointer to store the [3][2] component. Can be NULL.
 * @param[out] ww Pointer to store the [3][3] component. Can be NULL.
 */
EINA_API void
eina_matrix4_values_get(const Eina_Matrix4 *m,
                        double *xx, double *xy, double *xz, double *xw,
                        double *yx, double *yy, double *yz, double *yw,
                        double *zx, double *zy, double *zz, double *zw,
                        double *wx, double *wy, double *wz, double *ww)
{
   if (xx) *xx = MATRIX_XX(m);
   if (xy) *xy = MATRIX_XY(m);
   if (xz) *xz = MATRIX_XZ(m);
   if (xw) *xw = MATRIX_XW(m);
   if (yx) *yx = MATRIX_YX(m);
   if (yy) *yy = MATRIX_YY(m);
   if (yz) *yz = MATRIX_YZ(m);
   if (yw) *yw = MATRIX_YW(m);
   if (zx) *zx = MATRIX_ZX(m);
   if (zy) *zy = MATRIX_ZY(m);
   if (zz) *zz = MATRIX_ZZ(m);
   if (zw) *zw = MATRIX_ZW(m);
   if (wx) *wx = MATRIX_WX(m);
   if (wy) *wy = MATRIX_WY(m);
   if (wz) *wz = MATRIX_WZ(m);
   if (ww) *ww = MATRIX_WW(m);
}

/**
 * @internal
 * @brief Retrieves the individual component values of a 3x3 floating-point matrix
 * and converts them to Eina_F16p16 fixed-point format.
 *
 * @param[in] m The floating-point matrix to read from. Must not be NULL.
 * @param[out] xx Pointer to store the [0][0] component as Eina_F16p16. Can be NULL.
 * @param[out] xy Pointer to store the [0][1] component as Eina_F16p16. Can be NULL.
 * @param[out] xz Pointer to store the [0][2] component as Eina_F16p16. Can be NULL.
 * @param[out] yx Pointer to store the [1][0] component as Eina_F16p16. Can be NULL.
 * @param[out] yy Pointer to store the [1][1] component as Eina_F16p16. Can be NULL.
 * @param[out] yz Pointer to store the [1][2] component as Eina_F16p16. Can be NULL.
 * @param[out] zx Pointer to store the [2][0] component as Eina_F16p16. Can be NULL.
 * @param[out] zy Pointer to store the [2][1] component as Eina_F16p16. Can be NULL.
 * @param[out] zz Pointer to store the [2][2] component as Eina_F16p16. Can be NULL.
 */
EINA_API void
eina_matrix3_fixed_values_get(const Eina_Matrix3 *m,
                              Eina_F16p16 *xx, Eina_F16p16 *xy, Eina_F16p16 *xz,
                              Eina_F16p16 *yx, Eina_F16p16 *yy, Eina_F16p16 *yz,
                              Eina_F16p16 *zx, Eina_F16p16 *zy, Eina_F16p16 *zz)
{
   if (xx) *xx = eina_f16p16_double_from(MATRIX_XX(m));
   if (xy) *xy = eina_f16p16_double_from(MATRIX_XY(m));
   if (xz) *xz = eina_f16p16_double_from(MATRIX_XZ(m));
   if (yx) *yx = eina_f16p16_double_from(MATRIX_YX(m));
   if (yy) *yy = eina_f16p16_double_from(MATRIX_YY(m));
   if (yz) *yz = eina_f16p16_double_from(MATRIX_YZ(m));
   if (zx) *zx = eina_f16p16_double_from(MATRIX_ZX(m));
   if (zy) *zy = eina_f16p16_double_from(MATRIX_ZY(m));
   if (zz) *zz = eina_f16p16_double_from(MATRIX_ZZ(m));
}

/**
 * @internal
 * @brief Converts a 3x3 floating-point matrix to a 3x3 fixed-point (Eina_F16p16) matrix.
 *
 * @param[in] m The source floating-point matrix. Must not be NULL.
 * @param[out] fm The destination fixed-point matrix. Must not be NULL.
 */
EINA_API void
eina_matrix3_matrix3_f16p16_to(const Eina_Matrix3 *m,
                               Eina_Matrix3_F16p16 *fm)
{
   eina_matrix3_fixed_values_get(m,
                                 &fm->xx, &fm->xy, &fm->xz,
                                 &fm->yx, &fm->yy, &fm->yz,
                                 &fm->zx, &fm->zy, &fm->zz);
}

/**
 * @internal
 * @brief Transforms a 2D point (x, y) using a 3x3 matrix.
 * This function applies the transformation defined by matrix @p m to the
 * point (@p x, @p y). If the matrix is projective (zx or zy is non-zero),
 * a perspective division is performed.
 *
 * @param[in] m The 3x3 transformation matrix. Must not be NULL.
 * @param[in] x The x-coordinate of the point to transform.
 * @param[in] y The y-coordinate of the point to transform.
 * @param[out] xr Pointer to store the transformed x-coordinate. Can be NULL.
 * @param[out] yr Pointer to store the transformed y-coordinate. Can be NULL.
 */
EINA_API void
eina_matrix3_point_transform(const Eina_Matrix3 *m,
                             double x, double y,
                             double *xr, double *yr)
{
   double xrr, yrr;

   if (!EINA_DBL_EQ(MATRIX_ZX(m), 0.0) &&
       !EINA_DBL_EQ(MATRIX_ZY(m), 0.0))
     {
        xrr = (x * MATRIX_XX(m) + y * MATRIX_XY(m) + MATRIX_XZ(m));
        yrr = (x * MATRIX_YX(m) + y * MATRIX_YY(m) + MATRIX_YZ(m));
     }
   else
     {
        xrr = (x * MATRIX_XX(m) + y * MATRIX_XY(m) + MATRIX_XZ(m)) /
          (x * MATRIX_ZX(m) + y * MATRIX_ZY(m) + MATRIX_ZZ(m));
        yrr = (x * MATRIX_YX(m) + y * MATRIX_YY(m) + MATRIX_YZ(m)) /
          (x * MATRIX_ZX(m) + y * MATRIX_ZY(m) + MATRIX_ZZ(m));
     }

   if (xr) *xr = xrr;
   if (yr) *yr = yrr;
}

/**
 * @internal
 * @brief Transforms the four corners of a rectangle using a 3x3 matrix,
 * resulting in a quadrangle.
 *
 * @param[in] m The 3x3 transformation matrix. Must not be NULL.
 * @param[in] r The source rectangle. Its (x,y) are top-left, (x+w,y) top-right,
 *              (x+w,y+h) bottom-right, (x,y+h) bottom-left. Must not be NULL.
 * @param[out] q The destination quadrangle to store the transformed corner points.
 *               The points are stored in order: top-left (x0,y0), top-right (x1,y1),
 *               bottom-right (x2,y2), bottom-left (x3,y3). Must not be NULL.
 */
EINA_API void
eina_matrix3_rectangle_transform(const Eina_Matrix3 *m,
                                 const Eina_Rectangle *r,
                                 const Eina_Quad *q)
{
   eina_matrix3_point_transform(m, r->x, r->y, &((Eina_Quad *)q)->x0, &((Eina_Quad *)q)->y0);
   eina_matrix3_point_transform(m, r->x + r->w, r->y, &((Eina_Quad *)q)->x1, &((Eina_Quad *)q)->y1);
   eina_matrix3_point_transform(m, r->x + r->w, r->y + r->h, &((Eina_Quad *)q)->x2, &((Eina_Quad *)q)->y2);
   eina_matrix3_point_transform(m, r->x, r->y + r->h, &((Eina_Quad *)q)->x3, &((Eina_Quad *)q)->y3);
}

/**
 * @internal
 * @brief Computes the cofactor matrix of a given 3x3 matrix.
 * The cofactor of an element a_ij is C_ij = (-1)^(i+j) * M_ij, where M_ij
 * is the determinant of the submatrix obtained by deleting row i and column j.
 *
 * @param[in] m The source 3x3 matrix. Must not be NULL.
 * @param[out] a The resulting cofactor matrix. Must not be NULL.
 *               @p a can be the same as @p m.
 */
EINA_API void
eina_matrix3_cofactor(const Eina_Matrix3 *m, Eina_Matrix3 *a)
{
   double a11, a12, a13, a21, a22, a23, a31, a32, a33;

   a11 = (MATRIX_YY(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZY(m));
   a12 = -1 * ((MATRIX_YX(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZX(m)));
   a13 = (MATRIX_YX(m) * MATRIX_ZY(m)) - (MATRIX_YY(m) * MATRIX_ZX(m));

   a21 = -1 * ((MATRIX_XY(m) * MATRIX_ZZ(m)) - (MATRIX_XZ(m) * MATRIX_ZY(m)));
   a22 = (MATRIX_XX(m) * MATRIX_ZZ(m)) - (MATRIX_XZ(m) * MATRIX_ZX(m));
   a23 = -1 * ((MATRIX_XX(m) * MATRIX_ZY(m)) - (MATRIX_XY(m) * MATRIX_ZX(m)));

   a31 = (MATRIX_XY(m) * MATRIX_YZ(m)) - (MATRIX_XZ(m) * MATRIX_YY(m));
   a32 = -1 * ((MATRIX_XX(m) * MATRIX_YZ(m)) - (MATRIX_XZ(m) * MATRIX_YX(m)));
   a33 = (MATRIX_XX(m) * MATRIX_YY(m)) - (MATRIX_XY(m) * MATRIX_YX(m));

   MATRIX_XX(a) = a11;
   MATRIX_XY(a) = a12;
   MATRIX_XZ(a) = a13;

   MATRIX_YX(a) = a21;
   MATRIX_YY(a) = a22;
   MATRIX_YZ(a) = a23;

   MATRIX_ZX(a) = a31;
   MATRIX_ZY(a) = a32;
   MATRIX_ZZ(a) = a33;
}

/**
 * @internal
 * @brief Computes the transpose of a given 3x3 matrix.
 * The transpose of a matrix is obtained by swapping its rows and columns.
 *
 * @param[in] m The source 3x3 matrix. Must not be NULL.
 * @param[out] a The resulting transposed matrix. Must not be NULL.
 *               @p a can be the same as @p m if only diagonal elements are affected,
 *               but it's generally safer if @p a is distinct from @p m for transpose,
 *               though this implementation handles in-place for some elements.
 *               For a full in-place transpose, temporary variables would be needed
 *               if m and a are the same. This implementation is safe if m and a are distinct.
 *               If m and a are the same, it will correctly transpose.
 */
EINA_API void
eina_matrix3_transpose(const Eina_Matrix3 *m, Eina_Matrix3 *a)
{
   MATRIX_XX(a) = MATRIX_XX(m);
   MATRIX_XY(a) = MATRIX_YX(m);
   MATRIX_XZ(a) = MATRIX_ZX(m);

   MATRIX_YX(a) = MATRIX_XY(m);
   MATRIX_YY(a) = MATRIX_YY(m);
   MATRIX_YZ(a) = MATRIX_ZY(m);

   MATRIX_ZX(a) = MATRIX_XZ(m);
   MATRIX_ZY(a) = MATRIX_YZ(m);
   MATRIX_ZZ(a) = MATRIX_ZZ(m);
}

/**
 * @internal
 * @brief Computes the adjoint (or adjugate) of a given 3x3 matrix.
 * The adjoint of a matrix is the transpose of its cofactor matrix.
 *
 * @param[in] m The source 3x3 matrix. Must not be NULL.
 * @param[out] a The resulting adjoint matrix. Must not be NULL.
 *               @p a can be the same as @p m.
 */
EINA_API void
eina_matrix3_adjoint(const Eina_Matrix3 *m, Eina_Matrix3 *a)
{
   Eina_Matrix3 cofactor;

   /* cofactor */
   eina_matrix3_cofactor(m, &cofactor);
   /* transpose */
   eina_matrix3_transpose(&cofactor, a);
}

/**
 * @internal
 * @brief Calculates the determinant of a 3x3 matrix.
 *
 * @param[in] m The 3x3 matrix. Must not be NULL.
 * @return The determinant of the matrix.
 */
EINA_API double
eina_matrix3_determinant(const Eina_Matrix3 *m)
{
   double det;

   det = MATRIX_XX(m) * ((MATRIX_YY(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZY(m)));
   det -= MATRIX_XY(m) * ((MATRIX_YX(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZX(m)));
   det += MATRIX_XZ(m) * ((MATRIX_YX(m) * MATRIX_ZY(m)) - (MATRIX_YY(m) * MATRIX_ZX(m)));

   return det;
}

/**
 * @internal
 * @brief Divides all elements of a 3x3 matrix by a scalar value.
 *
 * @param[in,out] m The matrix to be divided. Must not be NULL.
 * @param[in] scalar The scalar value to divide by. Should not be zero.
 */
EINA_API void
eina_matrix3_divide(Eina_Matrix3 *m, double scalar)
{
   MATRIX_XX(m) /= scalar;
   MATRIX_XY(m) /= scalar;
   MATRIX_XZ(m) /= scalar;

   MATRIX_YX(m) /= scalar;
   MATRIX_YY(m) /= scalar;
   MATRIX_YZ(m) /= scalar;

   MATRIX_ZX(m) /= scalar;
   MATRIX_ZY(m) /= scalar;
   MATRIX_ZZ(m) /= scalar;
}

/**
 * @internal
 * @brief Computes the inverse of a 3x3 matrix.
 * The inverse is calculated as (1/determinant) * adjoint(matrix).
 * If the determinant is zero, the matrix is singular and cannot be inverted;
 * in this case, @p m2 is set to the identity matrix.
 *
 * @param[in] m The source 3x3 matrix to invert. Must not be NULL.
 * @param[out] m2 The resulting inverse matrix. Must not be NULL.
 *                @p m2 can be the same as @p m.
 */
EINA_API void
eina_matrix3_inverse(const Eina_Matrix3 *m, Eina_Matrix3 *m2)
{
   double scalar;

   /* determinant */
   scalar = eina_matrix3_determinant(m);
   if (EINA_DBL_EQ(scalar, 0.0))
     {
        eina_matrix3_identity(m2);
        return;
     }
   /* do its adjoint */
   eina_matrix3_adjoint(m, m2);
   /* divide */
   eina_matrix3_divide(m2, scalar);
}

/**
 * @internal
 * @brief Composes (multiplies) two 3x3 matrices (dst = m1 * m2).
 * Matrix multiplication is not commutative, so the order of @p m1 and @p m2 matters.
 *
 * @param[in] m1 The first matrix (left-hand side). Must not be NULL.
 * @param[in] m2 The second matrix (right-hand side). Must not be NULL.
 * @param[out] dst The resulting matrix. Must not be NULL.
 *                 @p dst can be the same as @p m1 or @p m2.
 */
EINA_API void
eina_matrix3_compose(const Eina_Matrix3 *m1,
                     const Eina_Matrix3 *m2,
                     Eina_Matrix3 *dst)
{
   double a11, a12, a13, a21, a22, a23, a31, a32, a33;

   a11 = (MATRIX_XX(m1) * MATRIX_XX(m2)) + (MATRIX_XY(m1) * MATRIX_YX(m2)) + (MATRIX_XZ(m1) * MATRIX_ZX(m2));
   a12 = (MATRIX_XX(m1) * MATRIX_XY(m2)) + (MATRIX_XY(m1) * MATRIX_YY(m2)) + (MATRIX_XZ(m1) * MATRIX_ZY(m2));
   a13 = (MATRIX_XX(m1) * MATRIX_XZ(m2)) + (MATRIX_XY(m1) * MATRIX_YZ(m2)) + (MATRIX_XZ(m1) * MATRIX_ZZ(m2));

   a21 = (MATRIX_YX(m1) * MATRIX_XX(m2)) + (MATRIX_YY(m1) * MATRIX_YX(m2)) + (MATRIX_YZ(m1) * MATRIX_ZX(m2));
   a22 = (MATRIX_YX(m1) * MATRIX_XY(m2)) + (MATRIX_YY(m1) * MATRIX_YY(m2)) + (MATRIX_YZ(m1) * MATRIX_ZY(m2));
   a23 = (MATRIX_YX(m1) * MATRIX_XZ(m2)) + (MATRIX_YY(m1) * MATRIX_YZ(m2)) + (MATRIX_YZ(m1) * MATRIX_ZZ(m2));

   a31 = (MATRIX_ZX(m1) * MATRIX_XX(m2)) + (MATRIX_ZY(m1) * MATRIX_YX(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZX(m2));
   a32 = (MATRIX_ZX(m1) * MATRIX_XY(m2)) + (MATRIX_ZY(m1) * MATRIX_YY(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZY(m2));
   a33 = (MATRIX_ZX(m1) * MATRIX_XZ(m2)) + (MATRIX_ZY(m1) * MATRIX_YZ(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZZ(m2));

   MATRIX_XX(dst) = a11;
   MATRIX_XY(dst) = a12;
   MATRIX_XZ(dst) = a13;
   MATRIX_YX(dst) = a21;
   MATRIX_YY(dst) = a22;
   MATRIX_YZ(dst) = a23;
   MATRIX_ZX(dst) = a31;
   MATRIX_ZY(dst) = a32;
   MATRIX_ZZ(dst) = a33;
}

/**
 * @internal
 * @brief Checks if two 3x3 matrices are equal within a small tolerance (EINA_DBL_EQ).
 *
 * @param[in] m1 The first matrix. Must not be NULL.
 * @param[in] m2 The second matrix. Must not be NULL.
 * @return EINA_TRUE if all corresponding elements are equal within tolerance,
 *         EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_matrix3_equal(const Eina_Matrix3 *m1, const Eina_Matrix3 *m2)
{
   if (!EINA_DBL_EQ(m1->xx, m2->xx) ||
       !EINA_DBL_EQ(m1->xy, m2->xy) ||
       !EINA_DBL_EQ(m1->xz, m2->xz) ||
       !EINA_DBL_EQ(m1->yx, m2->yx) ||
       !EINA_DBL_EQ(m1->yy, m2->yy) ||
       !EINA_DBL_EQ(m1->yz, m2->yz) ||
       !EINA_DBL_EQ(m1->zx, m2->zx) ||
       !EINA_DBL_EQ(m1->zy, m2->zy) ||
       !EINA_DBL_EQ(m1->zz, m2->zz))
     return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Composes (multiplies) two 3x3 fixed-point (Eina_F16p16) matrices (dst = m1 * m2).
 * Matrix multiplication is not commutative.
 *
 * @param[in] m1 The first fixed-point matrix (left-hand side). Must not be NULL.
 * @param[in] m2 The second fixed-point matrix (right-hand side). Must not be NULL.
 * @param[out] dst The resulting fixed-point matrix. Must not be NULL.
 *                 @p dst can be the same as @p m1 or @p m2.
 */
EINA_API void
eina_matrix3_f16p16_compose(const Eina_Matrix3_F16p16 *m1,
                            const Eina_Matrix3_F16p16 *m2,
                            Eina_Matrix3_F16p16 *dst)
{
   Eina_F16p16 a11, a12, a13, a21, a22, a23, a31, a32, a33;

   a11 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XX(m2)) +
     eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YX(m2)) +
     eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZX(m2));
   a12 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XY(m2)) +
     eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YY(m2)) +
     eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZY(m2));
   a13 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XZ(m2)) +
     eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YZ(m2)) +
     eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZZ(m2));

   a21 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XX(m2)) +
     eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YX(m2)) +
     eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZX(m2));
   a22 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XY(m2)) +
     eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YY(m2)) +
     eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZY(m2));
   a23 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XZ(m2)) +
     eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YZ(m2)) +
     eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZZ(m2));

   a31 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XX(m2)) +
     eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YX(m2)) +
     eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZX(m2));
   a32 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XY(m2)) +
     eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YY(m2)) +
     eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZY(m2));
   a33 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XZ(m2)) +
     eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YZ(m2)) +
     eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZZ(m2));

   MATRIX_XX(dst) = a11;
   MATRIX_XY(dst) = a12;
   MATRIX_XZ(dst) = a13;
   MATRIX_YX(dst) = a21;
   MATRIX_YY(dst) = a22;
   MATRIX_YZ(dst) = a23;
   MATRIX_ZX(dst) = a31;
   MATRIX_ZY(dst) = a32;
   MATRIX_ZZ(dst) = a33;
}

/**
 * @internal
 * @brief Applies a translation to a 3x3 matrix (m = m * translation_matrix).
 * The existing matrix @p m is post-multiplied by a translation matrix
 * created from @p tx and @p ty.
 *
 * @param[in,out] m The matrix to translate. Must not be NULL.
 * @param[in] tx The translation amount along the X-axis.
 * @param[in] ty The translation amount along the Y-axis.
 */
EINA_API void
eina_matrix3_translate(Eina_Matrix3 *m, double tx, double ty)
{
   Eina_Matrix3 tmp;
   MATRIX_XX(&tmp) = 1;
   MATRIX_XY(&tmp) = 0;
   MATRIX_XZ(&tmp) = tx;
   MATRIX_YX(&tmp) = 0;
   MATRIX_YY(&tmp) = 1;
   MATRIX_YZ(&tmp) = ty;
   MATRIX_ZX(&tmp) = 0;
   MATRIX_ZY(&tmp) = 0;
   MATRIX_ZZ(&tmp) = 1;
   eina_matrix3_compose(m, &tmp, m);
}

/**
 * @internal
 * @brief Applies a scaling operation to a 3x3 matrix (m = m * scale_matrix).
 * The existing matrix @p m is post-multiplied by a scaling matrix
 * created from @p sx and @p sy.
 *
 * @param[in,out] m The matrix to scale. Must not be NULL.
 * @param[in] sx The scaling factor along the X-axis.
 * @param[in] sy The scaling factor along the Y-axis.
 */
EINA_API void
eina_matrix3_scale(Eina_Matrix3 *m, double sx, double sy)
{
   Eina_Matrix3 tmp;
   MATRIX_XX(&tmp) = sx;
   MATRIX_XY(&tmp) = 0;
   MATRIX_XZ(&tmp) = 0;
   MATRIX_YX(&tmp) = 0;
   MATRIX_YY(&tmp) = sy;
   MATRIX_YZ(&tmp) = 0;
   MATRIX_ZX(&tmp) = 0;
   MATRIX_ZY(&tmp) = 0;
   MATRIX_ZZ(&tmp) = 1;
   eina_matrix3_compose(m, &tmp, m);
}

/**
 * @internal
 * @brief Applies a rotation to a 3x3 matrix (m = m * rotation_matrix).
 * The existing matrix @p m is post-multiplied by a 2D rotation matrix
 * created from the angle @p rad (in radians) around the Z-axis.
 *
 * @param[in,out] m The matrix to rotate. Must not be NULL.
 * @param[in] rad The rotation angle in radians.
 */
EINA_API void
eina_matrix3_rotate(Eina_Matrix3 *m, double rad)
{
   double c, s;

   /* Note: Local functions do not guarantee accuracy.
    *       Errors occur in the calculation of very small or very large numbers.
    *       Local cos and sin functions differ from the math header cosf and sinf functions
    *       by result values. The 4th decimal place is different.
    *       But local functions are certainly faster than functions in math library.
    *       Later we would want someone to look at this and improve accuracy.
    */
#if 1
   c = cos(rad);
   s = sin(rad);
#else
   /* normalize the angle between -pi,pi */
   rad = fmod(rad + M_PI, 2 * M_PI) - M_PI;
   c = _cos(rad);
   s = _sin(rad);
#endif

   Eina_Matrix3 tmp;
   MATRIX_XX(&tmp) = c;
   MATRIX_XY(&tmp) = -s;
   MATRIX_XZ(&tmp) = 0;
   MATRIX_YX(&tmp) = s;
   MATRIX_YY(&tmp) = c;
   MATRIX_YZ(&tmp) = 0;
   MATRIX_ZX(&tmp) = 0;
   MATRIX_ZY(&tmp) = 0;
   MATRIX_ZZ(&tmp) = 1;
   eina_matrix3_compose(m, &tmp, m);
}

/**
 * @internal
 * @brief Sets a 3x3 matrix to the identity matrix.
 * An identity matrix has 1s on the main diagonal and 0s elsewhere.
 *
 * @param[out] m The matrix to set to identity. Must not be NULL.
 */
EINA_API void
eina_matrix3_identity(Eina_Matrix3 *m)
{
   MATRIX_XX(m) = 1;
   MATRIX_XY(m) = 0;
   MATRIX_XZ(m) = 0;
   MATRIX_YX(m) = 0;
   MATRIX_YY(m) = 1;
   MATRIX_YZ(m) = 0;
   MATRIX_ZX(m) = 0;
   MATRIX_ZY(m) = 0;
   MATRIX_ZZ(m) = 1;
}

/**
 * @internal
 * @brief Sets a 3x3 fixed-point (Eina_F16p16) matrix to the identity matrix.
 * An identity matrix has 1.0 (represented as 65536 in Eina_F16p16) on the
 * main diagonal and 0s elsewhere.
 *
 * @param[out] m The fixed-point matrix to set to identity. Must not be NULL.
 */
EINA_API void
eina_matrix3_f16p16_identity(Eina_Matrix3_F16p16 *m)
{
   MATRIX_XX(m) = 65536;
   MATRIX_XY(m) = 0;
   MATRIX_XZ(m) = 0;
   MATRIX_YX(m) = 0;
   MATRIX_YY(m) = 65536;
   MATRIX_YZ(m) = 0;
   MATRIX_ZX(m) = 0;
   MATRIX_ZY(m) = 0;
   MATRIX_ZZ(m) = 65536;
}

/**
 * @internal
 * @brief Creates a 3x3 matrix that maps the unit square (0,0)-(1,1) to a given quadrangle.
 * The unit square corners are: (0,0), (1,0), (1,1), (0,1).
 * The quadrangle @p q defines the target coordinates for these corners.
 * If the quadrangle is a parallelogram, a simpler affine transformation is used.
 * Otherwise, a full projective transformation is computed.
 *
 * @param[out] m The resulting transformation matrix. Must not be NULL.
 * @param[in] q The target quadrangle. Its points (x0,y0) to (x3,y3) correspond
 *              to the transformed unit square corners in order:
 *              (0,0) -> (q->x0, q->y0)
 *              (1,0) -> (q->x1, q->y1)
 *              (0,1) -> (q->x3, q->y3) (Note: Evas mapping, (0,1) is often qx3,qy3)
 *              (1,1) -> (q->x2, q->y2)
 *              Must not be NULL.
 * @return EINA_TRUE if the matrix was successfully created, EINA_FALSE otherwise
 *         (e.g., if the quadrangle is degenerate in a way that prevents mapping).
 */
EINA_API Eina_Bool
eina_matrix3_square_quad_map(Eina_Matrix3 *m, const Eina_Quad *q)
{
   // ex = x0 - x1 + x2 - x3
   double ex = QUAD_X0(q) - QUAD_X1(q) + QUAD_X2(q) - QUAD_X3(q);
   // y0 - y1 + y2 - y3
   double ey = QUAD_Y0(q) - QUAD_Y1(q) + QUAD_Y2(q) - QUAD_Y3(q);

   /* parallelogram */
   if (EINA_DBL_EQ(ex, 0.0) && EINA_DBL_EQ(ey, 0.0))
     {
        /* create the affine matrix */
        MATRIX_XX(m) = QUAD_X1(q) - QUAD_X0(q);
        MATRIX_XY(m) = QUAD_X2(q) - QUAD_X1(q);
        MATRIX_XZ(m) = QUAD_X0(q);

        MATRIX_YX(m) = QUAD_Y1(q) - QUAD_Y0(q);
        MATRIX_YY(m) = QUAD_Y2(q) - QUAD_Y1(q);
        MATRIX_YZ(m) = QUAD_Y0(q);

        MATRIX_ZX(m) = 0;
        MATRIX_ZY(m) = 0;
        MATRIX_ZZ(m) = 1;

        return EINA_TRUE;
     }
   else
     {
        double dx1 = QUAD_X1(q) - QUAD_X2(q); // x1 - x2
        double dx2 = QUAD_X3(q) - QUAD_X2(q); // x3 - x2
        double dy1 = QUAD_Y1(q) - QUAD_Y2(q); // y1 - y2
        double dy2 = QUAD_Y3(q) - QUAD_Y2(q); // y3 - y2
        double den = (dx1 * dy2) - (dx2 * dy1);

        if (EINA_DBL_EQ(den, 0.0))
          return EINA_FALSE;

        MATRIX_ZX(m) = ((ex * dy2) - (dx2 * ey)) / den;
        MATRIX_ZY(m) = ((dx1 * ey) - (ex * dy1)) / den;
        MATRIX_ZZ(m) = 1;
        MATRIX_XX(m) = QUAD_X1(q) - QUAD_X0(q) + (MATRIX_ZX(m) * QUAD_X1(q));
        MATRIX_XY(m) = QUAD_X3(q) - QUAD_X0(q) + (MATRIX_ZY(m) * QUAD_X3(q));
        MATRIX_XZ(m) = QUAD_X0(q);
        MATRIX_YX(m) = QUAD_Y1(q) - QUAD_Y0(q) + (MATRIX_ZX(m) * QUAD_Y1(q));
        MATRIX_YY(m) = QUAD_Y3(q) - QUAD_Y0(q) + (MATRIX_ZY(m) * QUAD_Y3(q));
        MATRIX_YZ(m) = QUAD_Y0(q);

        return EINA_TRUE;
     }
}

/**
 * @internal
 * @brief Creates a 3x3 matrix that maps a given quadrangle to the unit square (0,0)-(1,1).
 * This is the inverse operation of eina_matrix3_square_quad_map().
 * The resulting matrix will have its zz component normalized to 1.0 if it's projective.
 *
 * @param[out] m The resulting transformation matrix. Must not be NULL.
 * @param[in] q The source quadrangle. Must not be NULL.
 * @return EINA_TRUE if the matrix was successfully created, EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_matrix3_quad_square_map(Eina_Matrix3 *m,
                             const Eina_Quad *q)
{
   Eina_Matrix3 tmp;

   /* compute square to quad */
   if (!eina_matrix3_square_quad_map(&tmp, q))
     return EINA_FALSE;

   eina_matrix3_inverse(&tmp, m);
   /* make the projective matrix3 always have 1 on zz */
   if (!EINA_DBL_EQ(MATRIX_ZZ(m), 1.0))
     {
        eina_matrix3_divide(m, MATRIX_ZZ(m));
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Creates a 3x3 matrix that maps a source quadrangle to a destination quadrangle.
 * This is achieved by first mapping the source quadrangle to the unit square,
 * and then mapping the unit square to the destination quadrangle.
 * M = map_unit_to_dst * map_src_to_unit
 *
 * @param[out] m The resulting transformation matrix. Must not be NULL.
 * @param[in] src The source quadrangle. Must not be NULL.
 * @param[in] dst The destination quadrangle. Must not be NULL.
 * @return EINA_TRUE if the matrix was successfully created, EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_matrix3_quad_quad_map(Eina_Matrix3 *m,
                           const Eina_Quad *src,
                           const Eina_Quad *dst)
{
   Eina_Matrix3 tmp;

   /* TODO check that both are actually quadrangles */
   if (!eina_matrix3_quad_square_map(m, src))
     return EINA_FALSE;
   if (!eina_matrix3_square_quad_map(&tmp, dst))
     return EINA_FALSE;
   eina_matrix3_compose(&tmp, m, m);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Converts a 4x4 matrix to a 3x3 matrix.
 * This is a projection, typically discarding the Z-axis information for 2D representation.
 * The 3x3 matrix is formed as follows:
 *   m3.xx = m4.xx, m3.xy = m4.xy, m3.xz = m4.xw (translation X)
 *   m3.yx = m4.yx, m3.yy = m4.yy, m3.yz = m4.yw (translation Y)
 *   m3.zx = m4.wx, m3.zy = m4.wy, m3.zz = m4.ww (perspective)
 *
 * @param[out] m3 The destination 3x3 matrix. Must not be NULL.
 * @param[in] m4 The source 4x4 matrix. Must not be NULL.
 */
EINA_API void
eina_matrix4_matrix3_to(Eina_Matrix3 *m3, const Eina_Matrix4 *m4)
{
   MATRIX_XX(m3) = MATRIX_XX(m4);
   MATRIX_XY(m3) = MATRIX_XY(m4);
   MATRIX_XZ(m3) = MATRIX_XW(m4);
   MATRIX_YX(m3) = MATRIX_YX(m4);
   MATRIX_YY(m3) = MATRIX_YY(m4);
   MATRIX_YZ(m3) = MATRIX_YW(m4);
   MATRIX_ZX(m3) = MATRIX_WX(m4);
   MATRIX_ZY(m3) = MATRIX_WY(m4);
   MATRIX_ZZ(m3) = MATRIX_WW(m4);
}

/**
 * @internal
 * @brief Converts a 3x3 matrix to a 4x4 matrix.
 * This typically embeds the 2D transformation of the 3x3 matrix into a 3D space,
 * setting Z-axis transformations to identity (no change in Z, no Z perspective).
 * The 4x4 matrix is formed as follows:
 *   m4.xx = m3.xx, m4.xy = m3.xy, m4.xz = 0,   m4.xw = m3.xz (translation X)
 *   m4.yx = m3.yx, m4.yy = m3.yy, m4.yz = 0,   m4.yw = m3.yz (translation Y)
 *   m4.zx = 0,     m4.zy = 0,     m4.zz = 1,   m4.zw = 0
 *   m4.wx = m3.zx, m4.wy = m3.zy, m4.wz = 0,   m4.ww = m3.zz (perspective)
 *
 * @param[out] m4 The destination 4x4 matrix. Must not be NULL.
 * @param[in] m3 The source 3x3 matrix. Must not be NULL.
 */
EINA_API void
eina_matrix3_matrix4_to(Eina_Matrix4 *m4, const Eina_Matrix3 *m3)
{
   MATRIX_XX(m4) = MATRIX_XX(m3);
   MATRIX_XY(m4) = MATRIX_XY(m3);
   MATRIX_XZ(m4) = 0;
   MATRIX_XW(m4) = MATRIX_XZ(m3);
   MATRIX_YX(m4) = MATRIX_YX(m3);
   MATRIX_YY(m4) = MATRIX_YY(m3);
   MATRIX_YZ(m4) = 0;
   MATRIX_YW(m4) = MATRIX_YZ(m3);
   MATRIX_ZX(m4) = 0;
   MATRIX_ZY(m4) = 0;
   MATRIX_ZZ(m4) = 1;
   MATRIX_ZW(m4) = 0;
   MATRIX_WX(m4) = MATRIX_ZX(m3);
   MATRIX_WY(m4) = MATRIX_ZY(m3);
   MATRIX_WZ(m4) = 0;
   MATRIX_WW(m4) = MATRIX_ZZ(m3);
}

/**
 * @internal
 * @brief Calculates the determinant of a 4x4 matrix.
 *
 * @param[in] m The 4x4 matrix. Must not be NULL.
 * @return The determinant of the matrix.
 */
EINA_API double
eina_matrix4_determinant(const Eina_Matrix4 *m)
{
   return
       MATRIX_XW(m) * MATRIX_YZ(m) * MATRIX_ZY(m) * MATRIX_WX(m)
     - MATRIX_XZ(m) * MATRIX_YW(m) * MATRIX_ZY(m) * MATRIX_WX(m)
     - MATRIX_XW(m) * MATRIX_YY(m) * MATRIX_ZZ(m) * MATRIX_WX(m)
     + MATRIX_XY(m) * MATRIX_YW(m) * MATRIX_ZZ(m) * MATRIX_WX(m)
     + MATRIX_XZ(m) * MATRIX_YY(m) * MATRIX_ZW(m) * MATRIX_WX(m)
     - MATRIX_XY(m) * MATRIX_YZ(m) * MATRIX_ZW(m) * MATRIX_WX(m)
     - MATRIX_XW(m) * MATRIX_YZ(m) * MATRIX_ZX(m) * MATRIX_WY(m)
     + MATRIX_XZ(m) * MATRIX_YW(m) * MATRIX_ZX(m) * MATRIX_WY(m)
     + MATRIX_XW(m) * MATRIX_YX(m) * MATRIX_ZZ(m) * MATRIX_WY(m)
     - MATRIX_XX(m) * MATRIX_YW(m) * MATRIX_ZZ(m) * MATRIX_WY(m)
     - MATRIX_XZ(m) * MATRIX_YX(m) * MATRIX_ZW(m) * MATRIX_WY(m)
     + MATRIX_XX(m) * MATRIX_YZ(m) * MATRIX_ZW(m) * MATRIX_WY(m)
     + MATRIX_XW(m) * MATRIX_YY(m) * MATRIX_ZX(m) * MATRIX_WZ(m)
     - MATRIX_XY(m) * MATRIX_YW(m) * MATRIX_ZX(m) * MATRIX_WZ(m)
     - MATRIX_XW(m) * MATRIX_YX(m) * MATRIX_ZY(m) * MATRIX_WZ(m)
     + MATRIX_XX(m) * MATRIX_YW(m) * MATRIX_ZY(m) * MATRIX_WZ(m)
     + MATRIX_XY(m) * MATRIX_YX(m) * MATRIX_ZW(m) * MATRIX_WZ(m)
     - MATRIX_XX(m) * MATRIX_YY(m) * MATRIX_ZW(m) * MATRIX_WZ(m)
     - MATRIX_XZ(m) * MATRIX_YY(m) * MATRIX_ZX(m) * MATRIX_WW(m)
     + MATRIX_XY(m) * MATRIX_YZ(m) * MATRIX_ZX(m) * MATRIX_WW(m)
     + MATRIX_XZ(m) * MATRIX_YX(m) * MATRIX_ZY(m) * MATRIX_WW(m)
     - MATRIX_XX(m) * MATRIX_YZ(m) * MATRIX_ZY(m) * MATRIX_WW(m)
     - MATRIX_XY(m) * MATRIX_YX(m) * MATRIX_ZZ(m) * MATRIX_WW(m)
     + MATRIX_XX(m) * MATRIX_YY(m) * MATRIX_ZZ(m) * MATRIX_WW(m);
}

/**
 * @internal
 * @brief Normalizes a 4x4 matrix by dividing all its elements by its determinant.
 * If the determinant is close to zero, normalization is not possible.
 *
 * @param[out] out The resulting normalized matrix. Must not be NULL.
 *                 Can be the same as @p in.
 * @param[in] in The source 4x4 matrix. Must not be NULL.
 * @return EINA_TRUE if normalization was successful (determinant was non-zero),
 *         EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_matrix4_normalized(Eina_Matrix4 *out, const Eina_Matrix4 *in)
{
   double det;

   det = eina_matrix4_determinant(in);
   if (fabs(det) < DBL_EPSILON) return EINA_FALSE;

   MATRIX_XX(out) = MATRIX_XX(in) / det;
   MATRIX_XY(out) = MATRIX_XY(in) / det;
   MATRIX_XZ(out) = MATRIX_XZ(in) / det;
   MATRIX_XW(out) = MATRIX_XW(in) / det;
   MATRIX_YX(out) = MATRIX_YX(in) / det;
   MATRIX_YY(out) = MATRIX_YY(in) / det;
   MATRIX_YZ(out) = MATRIX_YZ(in) / det;
   MATRIX_YW(out) = MATRIX_YW(in) / det;
   MATRIX_ZX(out) = MATRIX_ZX(in) / det;
   MATRIX_ZY(out) = MATRIX_ZY(in) / det;
   MATRIX_ZZ(out) = MATRIX_ZZ(in) / det;
   MATRIX_ZW(out) = MATRIX_ZW(in) / det;
   MATRIX_WX(out) = MATRIX_WX(in) / det;
   MATRIX_WY(out) = MATRIX_WY(in) / det;
   MATRIX_WZ(out) = MATRIX_WZ(in) / det;
   MATRIX_WW(out) = MATRIX_WW(in) / det;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Computes the inverse of a 4x4 matrix.
 * The inverse is calculated using the adjugate matrix method and division by determinant.
 * If the determinant is close to zero, the matrix is singular and cannot be inverted.
 *
 * @param[out] out The resulting inverse matrix. Must not be NULL.
 *                 Can be the same as @p in.
 * @param[in] in The source 4x4 matrix to invert. Must not be NULL.
 * @return EINA_TRUE if the matrix was successfully inverted (determinant non-zero),
 *         EINA_FALSE otherwise.
 */
EINA_API Eina_Bool
eina_matrix4_inverse(Eina_Matrix4 *out, const Eina_Matrix4 *in)
{
   double det;

   MATRIX_XX(out) =
       MATRIX_YY(in)  * MATRIX_ZZ(in) * MATRIX_WW(in)
     - MATRIX_YY(in)  * MATRIX_ZW(in) * MATRIX_WZ(in)
     - MATRIX_ZY(in)  * MATRIX_YZ(in)  * MATRIX_WW(in)
     + MATRIX_ZY(in)  * MATRIX_YW(in)  * MATRIX_WZ(in)
     + MATRIX_WY(in) * MATRIX_YZ(in)  * MATRIX_ZW(in)
     - MATRIX_WY(in) * MATRIX_YW(in)  * MATRIX_ZZ(in);

   MATRIX_YX(out) =
     - MATRIX_YX(in)  * MATRIX_ZZ(in) * MATRIX_WW(in)
     + MATRIX_YX(in)  * MATRIX_ZW(in) * MATRIX_WZ(in)
     + MATRIX_ZX(in)  * MATRIX_YZ(in)  * MATRIX_WW(in)
     - MATRIX_ZX(in)  * MATRIX_YW(in)  * MATRIX_WZ(in)
     - MATRIX_WX(in) * MATRIX_YZ(in)  * MATRIX_ZW(in)
     + MATRIX_WX(in) * MATRIX_YW(in)  * MATRIX_ZZ(in);

   MATRIX_ZX(out) =
       MATRIX_YX(in)  * MATRIX_ZY(in) * MATRIX_WW(in)
     - MATRIX_YX(in)  * MATRIX_ZW(in) * MATRIX_WY(in)
     - MATRIX_ZX(in)  * MATRIX_YY(in) * MATRIX_WW(in)
     + MATRIX_ZX(in)  * MATRIX_YW(in) * MATRIX_WY(in)
     + MATRIX_WX(in) * MATRIX_YY(in) * MATRIX_ZW(in)
     - MATRIX_WX(in) * MATRIX_YW(in) * MATRIX_ZY(in);

   MATRIX_WX(out) =
     - MATRIX_YX(in)  * MATRIX_ZY(in) * MATRIX_WZ(in)
     + MATRIX_YX(in)  * MATRIX_ZZ(in) * MATRIX_WY(in)
     + MATRIX_ZX(in)  * MATRIX_YY(in) * MATRIX_WZ(in)
     - MATRIX_ZX(in)  * MATRIX_YZ(in) * MATRIX_WY(in)
     - MATRIX_WX(in) * MATRIX_YY(in) * MATRIX_ZZ(in)
     + MATRIX_WX(in) * MATRIX_YZ(in) * MATRIX_ZY(in);

   MATRIX_XY(out) =
     - MATRIX_XY(in)  * MATRIX_ZZ(in) * MATRIX_WW(in)
     + MATRIX_XY(in)  * MATRIX_ZW(in) * MATRIX_WZ(in)
     + MATRIX_ZY(in)  * MATRIX_XZ(in) * MATRIX_WW(in)
     - MATRIX_ZY(in)  * MATRIX_XW(in) * MATRIX_WZ(in)
     - MATRIX_WY(in) * MATRIX_XZ(in) * MATRIX_ZW(in)
     + MATRIX_WY(in) * MATRIX_XW(in) * MATRIX_ZZ(in);

   MATRIX_YY(out) =
       MATRIX_XX(in)  * MATRIX_ZZ(in) * MATRIX_WW(in)
     - MATRIX_XX(in)  * MATRIX_ZW(in) * MATRIX_WZ(in)
     - MATRIX_ZX(in)  * MATRIX_XZ(in) * MATRIX_WW(in)
     + MATRIX_ZX(in)  * MATRIX_XW(in) * MATRIX_WZ(in)
     + MATRIX_WX(in) * MATRIX_XZ(in) * MATRIX_ZW(in)
     - MATRIX_WX(in) * MATRIX_XW(in) * MATRIX_ZZ(in);

   MATRIX_ZY(out) =
     - MATRIX_XX(in)  * MATRIX_ZY(in) * MATRIX_WW(in)
     + MATRIX_XX(in)  * MATRIX_ZW(in) * MATRIX_WY(in)
     + MATRIX_ZX(in)  * MATRIX_XY(in) * MATRIX_WW(in)
     - MATRIX_ZX(in)  * MATRIX_XW(in) * MATRIX_WY(in)
     - MATRIX_WX(in) * MATRIX_XY(in) * MATRIX_ZW(in)
     + MATRIX_WX(in) * MATRIX_XW(in) * MATRIX_ZY(in);

   MATRIX_WY(out) =
       MATRIX_XX(in)  * MATRIX_ZY(in) * MATRIX_WZ(in)
     - MATRIX_XX(in)  * MATRIX_ZZ(in) * MATRIX_WY(in)
     - MATRIX_ZX(in)  * MATRIX_XY(in) * MATRIX_WZ(in)
     + MATRIX_ZX(in)  * MATRIX_XZ(in) * MATRIX_WY(in)
     + MATRIX_WX(in) * MATRIX_XY(in) * MATRIX_ZZ(in)
     - MATRIX_WX(in) * MATRIX_XZ(in) * MATRIX_ZY(in);

   MATRIX_XZ(out) =
       MATRIX_XY(in)  * MATRIX_YZ(in) * MATRIX_WW(in)
     - MATRIX_XY(in)  * MATRIX_YW(in) * MATRIX_WZ(in)
     - MATRIX_YY(in)  * MATRIX_XZ(in) * MATRIX_WW(in)
     + MATRIX_YY(in)  * MATRIX_XW(in) * MATRIX_WZ(in)
     + MATRIX_WY(in) * MATRIX_XZ(in) * MATRIX_YW(in)
     - MATRIX_WY(in) * MATRIX_XW(in) * MATRIX_YZ(in);

   MATRIX_YZ(out) =
     - MATRIX_XX(in)  * MATRIX_YZ(in) * MATRIX_WW(in)
     + MATRIX_XX(in)  * MATRIX_YW(in) * MATRIX_WZ(in)
     + MATRIX_YX(in)  * MATRIX_XZ(in) * MATRIX_WW(in)
     - MATRIX_YX(in)  * MATRIX_XW(in) * MATRIX_WZ(in)
     - MATRIX_WX(in) * MATRIX_XZ(in) * MATRIX_YW(in)
     + MATRIX_WX(in) * MATRIX_XW(in) * MATRIX_YZ(in);

   MATRIX_ZZ(out) =
       MATRIX_XX(in)  * MATRIX_YY(in) * MATRIX_WW(in)
     - MATRIX_XX(in)  * MATRIX_YW(in) * MATRIX_WY(in)
     - MATRIX_YX(in)  * MATRIX_XY(in) * MATRIX_WW(in)
     + MATRIX_YX(in)  * MATRIX_XW(in) * MATRIX_WY(in)
     + MATRIX_WX(in) * MATRIX_XY(in) * MATRIX_YW(in)
     - MATRIX_WX(in) * MATRIX_XW(in) * MATRIX_YY(in);

   MATRIX_WZ(out) =
     - MATRIX_XX(in)  * MATRIX_YY(in) * MATRIX_WZ(in)
     + MATRIX_XX(in)  * MATRIX_YZ(in) * MATRIX_WY(in)
     + MATRIX_YX(in)  * MATRIX_XY(in) * MATRIX_WZ(in)
     - MATRIX_YX(in)  * MATRIX_XZ(in) * MATRIX_WY(in)
     - MATRIX_WX(in) * MATRIX_XY(in) * MATRIX_YZ(in)
     + MATRIX_WX(in) * MATRIX_XZ(in) * MATRIX_YY(in);

   MATRIX_XW(out) =
     - MATRIX_XY(in) * MATRIX_YZ(in) * MATRIX_ZW(in)
     + MATRIX_XY(in) * MATRIX_YW(in) * MATRIX_ZZ(in)
     + MATRIX_YY(in) * MATRIX_XZ(in) * MATRIX_ZW(in)
     - MATRIX_YY(in) * MATRIX_XW(in) * MATRIX_ZZ(in)
     - MATRIX_ZY(in) * MATRIX_XZ(in) * MATRIX_YW(in)
     + MATRIX_ZY(in) * MATRIX_XW(in) * MATRIX_YZ(in);

   MATRIX_YW(out) =
       MATRIX_XX(in) * MATRIX_YZ(in) * MATRIX_ZW(in)
     - MATRIX_XX(in) * MATRIX_YW(in) * MATRIX_ZZ(in)
     - MATRIX_YX(in) * MATRIX_XZ(in) * MATRIX_ZW(in)
     + MATRIX_YX(in) * MATRIX_XW(in) * MATRIX_ZZ(in)
     + MATRIX_ZX(in) * MATRIX_XZ(in) * MATRIX_YW(in)
     - MATRIX_ZX(in) * MATRIX_XW(in) * MATRIX_YZ(in);

   MATRIX_ZW(out) =
     - MATRIX_XX(in) * MATRIX_YY(in) * MATRIX_ZW(in)
     + MATRIX_XX(in) * MATRIX_YW(in) * MATRIX_ZY(in)
     + MATRIX_YX(in) * MATRIX_XY(in) * MATRIX_ZW(in)
     - MATRIX_YX(in) * MATRIX_XW(in) * MATRIX_ZY(in)
     - MATRIX_ZX(in) * MATRIX_XY(in) * MATRIX_YW(in)
     + MATRIX_ZX(in) * MATRIX_XW(in) * MATRIX_YY(in);

   MATRIX_WW(out) =
       MATRIX_XX(in) * MATRIX_YY(in) * MATRIX_ZZ(in)
     - MATRIX_XX(in) * MATRIX_YZ(in) * MATRIX_ZY(in)
     - MATRIX_YX(in) * MATRIX_XY(in) * MATRIX_ZZ(in)
     + MATRIX_YX(in) * MATRIX_XZ(in) * MATRIX_ZY(in)
     + MATRIX_ZX(in) * MATRIX_XY(in) * MATRIX_YZ(in)
     - MATRIX_ZX(in) * MATRIX_XZ(in) * MATRIX_YY(in);

   det =
       MATRIX_XX(in) * MATRIX_XX(out)
     + MATRIX_XY(in) * MATRIX_YX(out)
     + MATRIX_XZ(in) * MATRIX_ZX(out)
     + MATRIX_XW(in) * MATRIX_WX(out);

   if (fabs(det) < DBL_EPSILON) return EINA_FALSE;

   det = 1.0 / det;

   MATRIX_XX(out) = MATRIX_XX(out) * det;
   MATRIX_XY(out) = MATRIX_XY(out) * det;
   MATRIX_XZ(out) = MATRIX_XZ(out) * det;
   MATRIX_XW(out) = MATRIX_XW(out) * det;
   MATRIX_YX(out) = MATRIX_YX(out) * det;
   MATRIX_YY(out) = MATRIX_YY(out) * det;
   MATRIX_YZ(out) = MATRIX_YZ(out) * det;
   MATRIX_YW(out) = MATRIX_YW(out) * det;
   MATRIX_ZX(out) = MATRIX_ZX(out) * det;
   MATRIX_ZY(out) = MATRIX_ZY(out) * det;
   MATRIX_ZZ(out) = MATRIX_ZZ(out) * det;
   MATRIX_ZW(out) = MATRIX_ZW(out) * det;
   MATRIX_WX(out) = MATRIX_WX(out) * det;
   MATRIX_WY(out) = MATRIX_WY(out) * det;
   MATRIX_WZ(out) = MATRIX_WZ(out) * det;
   MATRIX_WW(out) = MATRIX_WW(out) * det;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Computes the transpose of a given 4x4 matrix.
 * The transpose of a matrix is obtained by swapping its rows and columns.
 *
 * @param[out] out The resulting transposed matrix. Must not be NULL.
 *                 Can be the same as @p in.
 * @param[in] in The source 4x4 matrix. Must not be NULL.
 */
EINA_API void
eina_matrix4_transpose(Eina_Matrix4 *out, const Eina_Matrix4 *in)
{
   MATRIX_XX(out) = MATRIX_XX(in);
   MATRIX_XY(out) = MATRIX_YX(in);
   MATRIX_XZ(out) = MATRIX_ZX(in);
   MATRIX_XW(out) = MATRIX_WX(in);
   MATRIX_YX(out) = MATRIX_XY(in);
   MATRIX_YY(out) = MATRIX_YY(in);
   MATRIX_YZ(out) = MATRIX_ZY(in);
   MATRIX_YW(out) = MATRIX_WY(in);
   MATRIX_ZX(out) = MATRIX_XZ(in);
   MATRIX_ZY(out) = MATRIX_YZ(in);
   MATRIX_ZZ(out) = MATRIX_ZZ(in);
   MATRIX_ZW(out) = MATRIX_WZ(in);
   MATRIX_WX(out) = MATRIX_XW(in);
   MATRIX_WY(out) = MATRIX_YW(in);
   MATRIX_WZ(out) = MATRIX_ZW(in);
   MATRIX_WW(out) = MATRIX_WW(in);
}

/**
 * @internal
 * @brief Multiplies two 4x4 matrices (out = mat_a * mat_b), safely handling
 * cases where @p out might be the same as @p mat_a or @p mat_b.
 * Uses a temporary matrix for intermediate calculations if @p out overlaps
 * with an input matrix.
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix4_multiply_copy(Eina_Matrix4 *out,
                      const Eina_Matrix4 *mat_a, const Eina_Matrix4 *mat_b)
{
   if (out != mat_a && out != mat_b)
     {
        eina_matrix4_multiply(out, mat_a, mat_b);
     }
   else
     {
        Eina_Matrix4 result;

        eina_matrix4_multiply(&result, mat_a, mat_b);
        eina_matrix4_copy(out, &result);
     }
}

/**
 * @internal
 * @brief Sets a 4x4 matrix to the identity matrix.
 * An identity matrix has 1.0s on the main diagonal and 0s elsewhere.
 *
 * @param[out] out The matrix to set to identity. Must not be NULL.
 */
EINA_API void
eina_matrix4_identity(Eina_Matrix4 *out)
{
   memset(out, 0, sizeof (Eina_Matrix4));

   MATRIX_XX(out) = 1.0;
   MATRIX_YY(out) = 1.0;
   MATRIX_ZZ(out) = 1.0;
   MATRIX_WW(out) = 1.0;
}

/**
 * @internal
 * @brief Retrieves the type of a 2x2 floating-point matrix.
 * This function checks if the matrix @p m is an identity matrix or a more
 * general affine matrix.
 *
 * @param m The matrix to analyze. Must not be NULL.
 * @return EINA_MATRIX_TYPE_IDENTITY if it's an identity matrix,
 *         otherwise EINA_MATRIX_TYPE_AFFINE.
 */
EINA_API Eina_Matrix_Type
eina_matrix2_type_get(const Eina_Matrix2 *m)
{
   if (EINA_DBL_EQ(MATRIX_XX(m), 1.0) &&
       EINA_DBL_EQ(MATRIX_XY(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_YX(m), 0.0) &&
       EINA_DBL_EQ(MATRIX_YY(m), 1.0))
     return EINA_MATRIX_TYPE_IDENTITY;
   return EINA_MATRIX_TYPE_AFFINE;
}

/**
 * @internal
 * @brief Sets the values of a 4x4 matrix from a C array of 16 doubles.
 * The array elements are copied in row-major order into the matrix structure.
 * Example: v = {xx, xy, xz, xw, yx, yy, yz, yw, ...}
 *
 * @param[out] m The matrix to set. Must not be NULL.
 * @param[in] v Pointer to an array of 16 double values. Must not be NULL.
 */
EINA_API void
eina_matrix4_array_set(Eina_Matrix4 *m, const double *v)
{
   memcpy(&MATRIX_XX(m), v, sizeof(double) * 16);
}

/**
 * @internal
 * @brief Copies the content of one 4x4 matrix to another.
 *
 * @param[out] dst The destination matrix. Must not be NULL.
 * @param[in] src The source matrix. Must not be NULL.
 */
EINA_API void
eina_matrix4_copy(Eina_Matrix4 *dst, const Eina_Matrix4 *src)
{
   memcpy(dst, src, sizeof(Eina_Matrix4));
}

/**
 * @internal
 * @brief Multiplies two 4x4 matrices (out = mat_a * mat_b).
 * This function provides an optimization: if either @p mat_a or @p mat_b
 * is an identity matrix, it performs a copy instead of a full multiplication.
 * Otherwise, it calls eina_matrix4_compose().
 * Note: This function assumes @p out is distinct from @p mat_a and @p mat_b
 * if a full multiplication occurs via eina_matrix4_compose. For safe in-place
 * operations or overlapping buffers, use eina_matrix4_multiply_copy().
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix4_multiply(Eina_Matrix4 *out, const Eina_Matrix4 *mat_a,
                           const Eina_Matrix4 *mat_b)
{
   if (eina_matrix4_type_get(mat_a) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix4_copy(out, mat_b);
        return;
     }

   if (eina_matrix4_type_get(mat_b) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix4_copy(out, mat_a);
        return;
     }

   eina_matrix4_compose(mat_a, mat_b, out);
}

/**
 * @internal
 * @brief Sets up a 4x4 orthographic projection matrix.
 * This matrix transforms coordinates from a 3D box defined by left, right,
 * bottom, top, dnear, and dfar into normalized device coordinates (-1 to 1
 * on each axis).
 *
 * @param[out] m The resulting orthographic projection matrix. Must not be NULL.
 * @param[in] left The coordinate for the left vertical clipping plane.
 * @param[in] right The coordinate for the right vertical clipping plane.
 * @param[in] bottom The coordinate for the bottom horizontal clipping plane.
 * @param[in] top The coordinate for the top horizontal clipping plane.
 * @param[in] dnear The distance to the near depth clipping plane.
 * @param[in] dfar The distance to the far depth clipping plane.
 */
EINA_API void
eina_matrix4_ortho_set(Eina_Matrix4 *m,
                    double left, double right, double bottom, double top,
                    double dnear, double dfar)
{
   double   w = right - left;
   double   h = top - bottom;
   double   depth = dnear - dfar;

   MATRIX_XX(m) = 2.0f / w;
   MATRIX_XY(m) = 0.0f;
   MATRIX_XZ(m) = 0.0f;
   MATRIX_XW(m) = 0.0f;

   MATRIX_YX(m) = 0.0f;
   MATRIX_YY(m) = 2.0f / h;
   MATRIX_YZ(m) = 0.0f;
   MATRIX_YW(m) = 0.0f;

   MATRIX_ZX(m) = 0.0f;
   MATRIX_ZY(m) = 0.0f;
   MATRIX_ZZ(m) = 2.0f / depth;
   MATRIX_ZW(m) = 0.0f;

   MATRIX_WX(m) = -(right + left) / w;
   MATRIX_WY(m) = -(top + bottom) / h;
   MATRIX_WZ(m) = (dfar + dnear) / depth;
   MATRIX_WW(m) = 1.0f;
}

/**
 * @internal
 * @brief Composes (multiplies) two 4x4 matrices (out = mat_a * mat_b).
 * Matrix multiplication is not commutative, so the order of @p mat_a and @p mat_b matters.
 * This is the core multiplication logic.
 *
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 * @param[out] out The resulting matrix. Must not be NULL.
 *                 @p out can be the same as @p mat_a or @p mat_b.
 */
EINA_API void
eina_matrix4_compose(const Eina_Matrix4 *mat_a,
                     const Eina_Matrix4 *mat_b,
                     Eina_Matrix4 *out)
{
   double xx, xy, xz, xw,
          yx, yy, yz, yw,
          zx, zy, zz, zw,
          wx, wy, wz, ww;

   xx = MATRIX_XX(mat_a) * MATRIX_XX(mat_b) + MATRIX_XY(mat_a) * MATRIX_YX(mat_b) +
        MATRIX_XZ(mat_a) * MATRIX_ZX(mat_b) + MATRIX_XW(mat_a) * MATRIX_WX(mat_b);
   xy = MATRIX_XX(mat_a) * MATRIX_XY(mat_b) + MATRIX_XY(mat_a) * MATRIX_YY(mat_b) +
        MATRIX_XZ(mat_a) * MATRIX_ZY(mat_b) + MATRIX_XW(mat_a) * MATRIX_WY(mat_b);
   xz = MATRIX_XX(mat_a) * MATRIX_XZ(mat_b) + MATRIX_XY(mat_a) * MATRIX_YZ(mat_b) +
        MATRIX_XZ(mat_a) * MATRIX_ZZ(mat_b) + MATRIX_XW(mat_a) * MATRIX_WZ(mat_b);
   xw = MATRIX_XX(mat_a) * MATRIX_XW(mat_b) + MATRIX_XY(mat_a) * MATRIX_YW(mat_b) +
        MATRIX_XZ(mat_a) * MATRIX_ZW(mat_b) + MATRIX_XW(mat_a) * MATRIX_WW(mat_b);

   yx = MATRIX_YX(mat_a) * MATRIX_XX(mat_b) + MATRIX_YY(mat_a) * MATRIX_YX(mat_b) +
        MATRIX_YZ(mat_a) * MATRIX_ZX(mat_b) + MATRIX_YW(mat_a) * MATRIX_WX(mat_b);
   yy = MATRIX_YX(mat_a) * MATRIX_XY(mat_b) + MATRIX_YY(mat_a) * MATRIX_YY(mat_b) +
        MATRIX_YZ(mat_a) * MATRIX_ZY(mat_b) + MATRIX_YW(mat_a) * MATRIX_WY(mat_b);
   yz = MATRIX_YX(mat_a) * MATRIX_XZ(mat_b) + MATRIX_YY(mat_a) * MATRIX_YZ(mat_b) +
        MATRIX_YZ(mat_a) * MATRIX_ZZ(mat_b) + MATRIX_YW(mat_a) * MATRIX_WZ(mat_b);
   yw = MATRIX_YX(mat_a) * MATRIX_XW(mat_b) + MATRIX_YY(mat_a) * MATRIX_YW(mat_b) +
        MATRIX_YZ(mat_a) * MATRIX_ZW(mat_b) + MATRIX_YW(mat_a) * MATRIX_WW(mat_b);

   zx = MATRIX_ZX(mat_a) * MATRIX_XX(mat_b) + MATRIX_ZY(mat_a) * MATRIX_YX(mat_b) +
        MATRIX_ZZ(mat_a) * MATRIX_ZX(mat_b) + MATRIX_ZW(mat_a) * MATRIX_WX(mat_b);
   zy = MATRIX_ZX(mat_a) * MATRIX_XY(mat_b) + MATRIX_ZY(mat_a) * MATRIX_YY(mat_b) +
        MATRIX_ZZ(mat_a) * MATRIX_ZY(mat_b) + MATRIX_ZW(mat_a) * MATRIX_WY(mat_b);
   zz = MATRIX_ZX(mat_a) * MATRIX_XZ(mat_b) + MATRIX_ZY(mat_a) * MATRIX_YZ(mat_b) +
        MATRIX_ZZ(mat_a) * MATRIX_ZZ(mat_b) + MATRIX_ZW(mat_a) * MATRIX_WZ(mat_b);
   zw = MATRIX_ZX(mat_a) * MATRIX_XW(mat_b) + MATRIX_ZY(mat_a) * MATRIX_YW(mat_b) +
        MATRIX_ZZ(mat_a) * MATRIX_ZW(mat_b) + MATRIX_ZW(mat_a) * MATRIX_WW(mat_b);

   wx = MATRIX_WX(mat_a) * MATRIX_XX(mat_b) + MATRIX_WY(mat_a) * MATRIX_YX(mat_b) +
        MATRIX_WZ(mat_a) * MATRIX_ZX(mat_b) + MATRIX_WW(mat_a) * MATRIX_WX(mat_b);
   wy = MATRIX_WX(mat_a) * MATRIX_XY(mat_b) + MATRIX_WY(mat_a) * MATRIX_YY(mat_b) +
        MATRIX_WZ(mat_a) * MATRIX_ZY(mat_b) + MATRIX_WW(mat_a) * MATRIX_WY(mat_b);
   wz = MATRIX_WX(mat_a) * MATRIX_XZ(mat_b) + MATRIX_WY(mat_a) * MATRIX_YZ(mat_b) +
        MATRIX_WZ(mat_a) * MATRIX_ZZ(mat_b) + MATRIX_WW(mat_a) * MATRIX_WZ(mat_b);
   ww = MATRIX_WX(mat_a) * MATRIX_XW(mat_b) + MATRIX_WY(mat_a) * MATRIX_YW(mat_b) +
        MATRIX_WZ(mat_a) * MATRIX_ZW(mat_b) + MATRIX_WW(mat_a) * MATRIX_WW(mat_b);

   MATRIX_XX(out) = xx;
   MATRIX_XY(out) = xy;
   MATRIX_XZ(out) = xz;
   MATRIX_XW(out) = xw;

   MATRIX_YX(out) = yx;
   MATRIX_YY(out) = yy;
   MATRIX_YZ(out) = yz;
   MATRIX_YW(out) = yw;

   MATRIX_ZX(out) = zx;
   MATRIX_ZY(out) = zy;
   MATRIX_ZZ(out) = zz;
   MATRIX_ZW(out) = zw;

   MATRIX_WX(out) = wx;
   MATRIX_WY(out) = wy;
   MATRIX_WZ(out) = wz;
   MATRIX_WW(out) = ww;
}

/**
 * @internal
 * @brief Applies a translation to a 4x4 matrix (t = translation_matrix * t).
 * The existing matrix @p t is pre-multiplied by a translation matrix
 * created from @p tx, @p ty, and @p tz.
 * Note the order of multiplication: eina_matrix4_compose(&tmp, t, t) means t_new = tmp * t_old.
 *
 * @param[in,out] t The matrix to translate. Must not be NULL.
 * @param[in] tx The translation amount along the X-axis.
 * @param[in] ty The translation amount along the Y-axis.
 * @param[in] tz The translation amount along the Z-axis.
 */
EINA_API void
eina_matrix4_translate(Eina_Matrix4 *t, double tx, double ty, double tz)
{
   Eina_Matrix4 tmp;
   MATRIX_XX(&tmp) = 1;
   MATRIX_XY(&tmp) = 0;
   MATRIX_XZ(&tmp) = 0;
   MATRIX_XW(&tmp) = tx;

   MATRIX_YX(&tmp) = 0;
   MATRIX_YY(&tmp) = 1;
   MATRIX_YZ(&tmp) = 0;
   MATRIX_YW(&tmp) = ty;

   MATRIX_ZX(&tmp) = 0;
   MATRIX_ZY(&tmp) = 0;
   MATRIX_ZZ(&tmp) = 1;
   MATRIX_ZW(&tmp) = tz;

   MATRIX_WX(&tmp) = 0;
   MATRIX_WY(&tmp) = 0;
   MATRIX_WZ(&tmp) = 0;
   MATRIX_WW(&tmp) = 1;

   eina_matrix4_compose(&tmp, t, t);
}

/**
 * @internal
 * @brief Applies a scaling operation to a 4x4 matrix (t = scale_matrix * t).
 * The existing matrix @p t is pre-multiplied by a scaling matrix
 * created from @p sx, @p sy, and @p sz.
 * Note the order of multiplication: eina_matrix4_compose(&tmp, t, t) means t_new = tmp * t_old.
 *
 * @param[in,out] t The matrix to scale. Must not be NULL.
 * @param[in] sx The scaling factor along the X-axis.
 * @param[in] sy The scaling factor along the Y-axis.
 * @param[in] sz The scaling factor along the Z-axis.
 */
EINA_API void
eina_matrix4_scale(Eina_Matrix4 *t, double sx, double sy, double sz)
{
   Eina_Matrix4 tmp;
   MATRIX_XX(&tmp) = sx;
   MATRIX_XY(&tmp) = 0;
   MATRIX_XZ(&tmp) = 0;
   MATRIX_XW(&tmp) = 0;

   MATRIX_YX(&tmp) = 0;
   MATRIX_YY(&tmp) = sy;
   MATRIX_YZ(&tmp) = 0;
   MATRIX_YW(&tmp) = 0;

   MATRIX_ZX(&tmp) = 0;
   MATRIX_ZY(&tmp) = 0;
   MATRIX_ZZ(&tmp) = sz;
   MATRIX_ZW(&tmp) = 0;

   MATRIX_WX(&tmp) = 0;
   MATRIX_WY(&tmp) = 0;
   MATRIX_WZ(&tmp) = 0;
   MATRIX_WW(&tmp) = 1;

   eina_matrix4_compose(&tmp, t, t);
}

/**
 * @internal
 * @brief Applies a rotation to a 4x4 matrix (t = rotation_matrix * t).
 * The existing matrix @p t is pre-multiplied by a rotation matrix
 * created from the angle @p rad (in radians) around the specified @p axis.
 * Note the order of multiplication: eina_matrix4_compose(&tmp, t, t) means t_new = tmp * t_old.
 *
 * @param[in,out] t The matrix to rotate. Must not be NULL.
 * @param[in] rad The rotation angle in radians.
 * @param[in] axis The axis of rotation (EINA_MATRIX_AXIS_X, EINA_MATRIX_AXIS_Y, or EINA_MATRIX_AXIS_Z).
 */
EINA_API void
eina_matrix4_rotate(Eina_Matrix4 *t, double rad, Eina_Matrix_Axis axis)
{
   double c, s;

   /* Note: Local functions do not guarantee accuracy.
    *       Errors occur in the calculation of very small or very large numbers.
    *       Local cos and sin functions differ from the math header cosf and sinf functions
    *       by result values. The 4th decimal place is different.
    *       But local functions are certainly faster than functions in math library.
    *       Later we would want someone to look at this and improve accuracy.
    */
#if 1
   c = cos(rad);
   s = sin(rad);
#else
   /* normalize the angle between -pi,pi */
   rad = fmod(rad + M_PI, 2 * M_PI) - M_PI;
   c = _cos(rad);
   s = _sin(rad);
#endif

   Eina_Matrix4 tmp;
   eina_matrix4_identity(&tmp);

   switch (axis)
     {
        case EINA_MATRIX_AXIS_X:
          MATRIX_YY(&tmp) = c;
          MATRIX_YZ(&tmp) = -s;
          MATRIX_ZY(&tmp) = s;
          MATRIX_ZZ(&tmp) = c;
          break;
        case EINA_MATRIX_AXIS_Y:
          MATRIX_XX(&tmp) = c;
          MATRIX_XZ(&tmp) = s;
          MATRIX_ZX(&tmp) = -s;
          MATRIX_ZZ(&tmp) = c;
          break;
        case EINA_MATRIX_AXIS_Z:
          MATRIX_XX(&tmp) = c;
          MATRIX_XY(&tmp) = -s;
          MATRIX_YX(&tmp) = s;
          MATRIX_YY(&tmp) = c;
          break;
     }
   eina_matrix4_compose(&tmp, t, t);
}

/**
 * @internal
 * @brief Sets the values of a 3x3 matrix from a C array of 9 doubles.
 * The array elements are copied in row-major order into the matrix structure.
 * Example: v = {xx, xy, xz, yx, yy, yz, zx, zy, zz}
 *
 * @param[out] m The matrix to set. Must not be NULL.
 * @param[in] v Pointer to an array of 9 double values. Must not be NULL.
 */
EINA_API void
eina_matrix3_array_set(Eina_Matrix3 *m, const double *v)
{
   memcpy(&MATRIX_XX(m), v, sizeof(double) * 9);
}

/**
 * @internal
 * @brief Copies the content of one 3x3 matrix to another.
 *
 * @param[out] dst The destination matrix. Must not be NULL.
 * @param[in] src The source matrix. Must not be NULL.
 */
EINA_API void
eina_matrix3_copy(Eina_Matrix3 *dst, const Eina_Matrix3 *src)
{
   memcpy(dst, src, sizeof(Eina_Matrix3));
}

/**
 * @internal
 * @brief Multiplies two 3x3 matrices (out = mat_a * mat_b).
 * This function provides an optimization: if either @p mat_a or @p mat_b
 * is an identity matrix, it performs a copy instead of a full multiplication.
 * Otherwise, it calls eina_matrix3_compose().
 * Note: This function assumes @p out is distinct from @p mat_a and @p mat_b
 * if a full multiplication occurs via eina_matrix3_compose. For safe in-place
 * operations or overlapping buffers, use eina_matrix3_multiply_copy().
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix3_multiply(Eina_Matrix3 *out, const Eina_Matrix3 *mat_a, const Eina_Matrix3 *mat_b)
{
   if (eina_matrix3_type_get(mat_a) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix3_copy(out, mat_b);
        return;
     }

   if (eina_matrix3_type_get(mat_b) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix3_copy(out, mat_a);
        return;
     }

   eina_matrix3_compose(mat_a, mat_b, out);
}

/**
 * @internal
 * @brief Multiplies two 3x3 matrices (out = mat_a * mat_b), safely handling
 * cases where @p out might be the same as @p mat_a or @p mat_b.
 * Uses a temporary matrix for intermediate calculations if @p out overlaps
 * with an input matrix.
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix3_multiply_copy(Eina_Matrix3 *out, const Eina_Matrix3 *mat_a, const Eina_Matrix3 *mat_b)
{
   if (out != mat_a && out != mat_b)
     {
        eina_matrix3_multiply(out, mat_a, mat_b);
     }
   else
     {
        Eina_Matrix3 tmp;

        eina_matrix3_multiply(&tmp, mat_a, mat_b);
        eina_matrix3_copy(out, &tmp);
     }
}

/**
 * @internal
 * @brief Creates a 3x3 matrix representing only a translation (position).
 * The matrix is initialized to identity, then its translation components
 * (xz, yz) are set to @p p_x and @p p_y.
 *
 * @param[out] out The resulting translation matrix. Must not be NULL.
 * @param[in] p_x The translation amount along the X-axis.
 * @param[in] p_y The translation amount along the Y-axis.
 */
EINA_API void
eina_matrix3_position_transform_set(Eina_Matrix3 *out, const double p_x,
								 const double p_y)
{
   eina_matrix3_identity(out);
   MATRIX_XZ(out) = p_x;
   MATRIX_YZ(out) = p_y;
}

/**
 * @internal
 * @brief Creates a 3x3 matrix representing only a scaling operation.
 * The matrix is initialized to identity, then its scaling components
 * (xx, yy) are set to @p s_x and @p s_y.
 *
 * @param[out] out The resulting scaling matrix. Must not be NULL.
 * @param[in] s_x The scaling factor along the X-axis.
 * @param[in] s_y The scaling factor along the Y-axis.
 */
EINA_API void
eina_matrix3_scale_transform_set(Eina_Matrix3 *out, double s_x, double s_y)
{
   eina_matrix3_identity(out);
   MATRIX_XX(out) = s_x;
   MATRIX_YY(out) = s_y;
}

/**
 * @internal
 * @brief Calculates the normal matrix from a 4x4 modelview matrix.
 * The normal matrix is the transpose of the inverse of the upper-left 3x3
 * portion of the modelview matrix @p m. It's used to transform normal vectors
 * correctly when the modelview matrix involves non-uniform scaling.
 *
 * @param[out] out The resulting 3x3 normal matrix. Must not be NULL.
 * @param[in] m The source 4x4 modelview matrix. Must not be NULL.
 */
EINA_API void
eina_normal3_matrix_get(Eina_Matrix3 *out, const Eina_Matrix4 *m)
{
   /* Normal matrix is a transposed matrix of inversed modelview.
    * And we need only upper-left 3x3 terms to work with. */

   double   det;

   double   a = MATRIX_XX(m);
   double   b = MATRIX_YX(m);
   double   c = MATRIX_ZX(m);

   double   d = MATRIX_XY(m);
   double   e = MATRIX_YY(m);
   double   f = MATRIX_ZY(m);

   double   g = MATRIX_XZ(m);
   double   h = MATRIX_YZ(m);
   double   i = MATRIX_ZZ(m);

   det = a * e * i + b * f * g + c * d * h - g * e * c - h * f * a - i * d * b;

   if (fabs(det) >= DBL_EPSILON) det = 1.0 / det;
   else det = 0.0;

   MATRIX_XX(out) = (e * i - f * h) * det;
   MATRIX_XY(out) = (h * c - i * b) * det;
   MATRIX_XZ(out) = (b * f - c * e) * det;

   MATRIX_YX(out) = (g * f - d * i) * det;
   MATRIX_YY(out) = (a * i - g * c) * det;
   MATRIX_YZ(out) = (d * c - a * f) * det;

   MATRIX_ZX(out) = (d * h - g * e) * det;
   MATRIX_ZY(out) = (g * b - a * h) * det;
   MATRIX_ZZ(out) = (a * e - d * b) * det;
}

/**
 * @internal
 * @brief Sets the individual component values of a 2x2 floating-point matrix.
 *
 * @param[out] m The matrix to modify. Must not be NULL.
 * @param[in] xx Value for the [0][0] component.
 * @param[in] xy Value for the [0][1] component.
 * @param[in] yx Value for the [1][0] component.
 * @param[in] yy Value for the [1][1] component.
 */
EINA_API void
eina_matrix2_values_set(Eina_Matrix2 *m,
                        double xx, double xy,
                        double yx, double yy)
{
   MATRIX_XX(m) = xx;
   MATRIX_XY(m) = xy;
   MATRIX_YX(m) = yx;
   MATRIX_YY(m) = yy;
}

/**
 * @internal
 * @brief Retrieves the individual component values of a 2x2 floating-point matrix.
 *
 * @param[in] m The matrix to read from. Must not be NULL.
 * @param[out] xx Pointer to store the [0][0] component. Can be NULL.
 * @param[out] xy Pointer to store the [0][1] component. Can be NULL.
 * @param[out] yx Pointer to store the [1][0] component. Can be NULL.
 * @param[out] yy Pointer to store the [1][1] component. Can be NULL.
 */
EINA_API void
eina_matrix2_values_get(const Eina_Matrix2 *m,
                        double *xx, double *xy,
                        double *yx, double *yy)
{
   if (xx) *xx = MATRIX_XX(m);
   if (xy) *xy = MATRIX_XY(m);
   if (yx) *yx = MATRIX_YX(m);
   if (yy) *yy = MATRIX_YY(m);
}

/**
 * @internal
 * @brief Computes the inverse of a 2x2 matrix.
 * If the matrix is identity, it's copied directly.
 * If the determinant is zero, the matrix is singular, and the function returns
 * without modifying @p out (it should ideally set to identity or return a status).
 * Current behavior: if det is 0, @p out is not modified from its previous state.
 *
 * @param[out] out The resulting inverse matrix. Must not be NULL.
 *                 Can be the same as @p mat.
 * @param[in] mat The source 2x2 matrix to invert. Must not be NULL.
 */
EINA_API void
eina_matrix2_inverse(Eina_Matrix2 *out, const Eina_Matrix2 *mat)
{
   double         det;

   if (eina_matrix2_type_get(mat) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix2_copy(out, mat);
        return;
     }

   det = MATRIX_XX(mat) * MATRIX_YY(mat) - MATRIX_YX(mat) * MATRIX_XY(mat);

   if (EINA_DBL_EQ(det, 0.0))
     return;

   det = 1.0 / det;

   MATRIX_XX(out) =  MATRIX_YY(mat) * det;
   MATRIX_XY(out) = -MATRIX_XY(mat) * det;
   MATRIX_YX(out) = -MATRIX_YX(mat) * det;
   MATRIX_YY(out) =  MATRIX_XX(mat) * det;
}

/**
 * @internal
 * @brief Sets a 2x2 matrix to the identity matrix.
 * An identity matrix has 1.0s on the main diagonal and 0s elsewhere.
 *
 * @param[out] m The matrix to set to identity. Must not be NULL.
 */
EINA_API void
eina_matrix2_identity(Eina_Matrix2 *m)
{
   MATRIX_XX(m) = 1.0;
   MATRIX_XY(m) = 0.0;

   MATRIX_YX(m) = 0.0;
   MATRIX_YY(m) = 1.0;
}

/**
 * @internal
 * @brief Sets the values of a 2x2 matrix from a C array of 4 doubles.
 * The array elements are copied in row-major order into the matrix structure.
 * Example: v = {xx, xy, yx, yy}
 *
 * @param[out] m The matrix to set. Must not be NULL.
 * @param[in] v Pointer to an array of 4 double values. Must not be NULL.
 */
EINA_API void
eina_matrix2_array_set(Eina_Matrix2 *m, const double *v)
{
   memcpy(&MATRIX_XX(m), v, sizeof(double) * 4);
}

/**
 * @internal
 * @brief Copies the content of one 2x2 matrix to another.
 *
 * @param[out] dst The destination matrix. Must not be NULL.
 * @param[in] src The source matrix. Must not be NULL.
 */
EINA_API void
eina_matrix2_copy(Eina_Matrix2 *dst, const Eina_Matrix2 *src)
{
   memcpy(dst, src, sizeof(Eina_Matrix2));
}

/**
 * @internal
 * @brief Multiplies two 2x2 matrices (out = mat_a * mat_b).
 * This function provides an optimization: if either @p mat_a or @p mat_b
 * is an identity matrix, it performs a copy instead of a full multiplication.
 * Otherwise, it performs the direct multiplication.
 * Note: This function assumes @p out is distinct from @p mat_a and @p mat_b
 * for the direct multiplication part. For safe in-place operations or
 * overlapping buffers, use eina_matrix2_multiply_copy().
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix2_multiply(Eina_Matrix2 *out, const Eina_Matrix2 *mat_a, const Eina_Matrix2 *mat_b)
{
   if (eina_matrix2_type_get(mat_a) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix2_copy(out, mat_b);
        return;
     }

   if (eina_matrix2_type_get(mat_b) == EINA_MATRIX_TYPE_IDENTITY)
     {
        eina_matrix2_copy(out, mat_a);
        return;
     }

   MATRIX_XX(out) = MATRIX_XX(mat_a) * MATRIX_XX(mat_b) + MATRIX_YX(mat_a) * MATRIX_XY(mat_b);
   MATRIX_YX(out) = MATRIX_XX(mat_a) * MATRIX_YX(mat_b) + MATRIX_YX(mat_a) * MATRIX_YY(mat_b);

   MATRIX_XY(out) = MATRIX_XY(mat_a) * MATRIX_XX(mat_b) + MATRIX_YY(mat_a) * MATRIX_XY(mat_b);
   MATRIX_YY(out) = MATRIX_XY(mat_a) * MATRIX_YX(mat_b) + MATRIX_YY(mat_a) * MATRIX_YY(mat_b);
}

/**
 * @internal
 * @brief Multiplies two 2x2 matrices (out = mat_a * mat_b), safely handling
 * cases where @p out might be the same as @p mat_a or @p mat_b.
 * Uses a temporary matrix for intermediate calculations if @p out overlaps
 * with an input matrix.
 *
 * @param[out] out The resulting matrix. Must not be NULL.
 * @param[in] mat_a The first matrix (left-hand side). Must not be NULL.
 * @param[in] mat_b The second matrix (right-hand side). Must not be NULL.
 */
EINA_API void
eina_matrix2_multiply_copy(Eina_Matrix2 *out, const Eina_Matrix2 *mat_a, const Eina_Matrix2 *mat_b)
{
   if (out != mat_a && out != mat_b)
     {
        eina_matrix2_multiply(out, mat_a, mat_b);
     }
   else
     {
        Eina_Matrix2 tmp;

        eina_matrix2_multiply(&tmp, mat_a, mat_b);
        eina_matrix2_copy(out, &tmp);
     }
}
