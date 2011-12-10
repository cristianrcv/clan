
   /*+------- <| --------------------------------------------------------**
    **         A                     Clan                                **
    **---     /.\   -----------------------------------------------------**
    **   <|  [""M#                 relation.c                              **
    **-   A   | #   -----------------------------------------------------**
    **   /.\ [""M#         First version: 30/04/2008                     **
    **- [""M# | #  U"U#U  -----------------------------------------------**
         | #  | #  \ .:/
         | #  | #___| #
 ******  | "--'     .-"  ******************************************************
 *     |"-"-"-"-"-#-#-##   Clan : the Chunky Loop Analyzer (experimental)     *
 ****  |     # ## ######  *****************************************************
 *      \       .::::'/                                                       *
 *       \      ::::'/     Copyright (C) 2008 University Paris-Sud 11         *
 *     :8a|    # # ##                                                         *
 *     ::88a      ###      This is free software; you can redistribute it     *
 *    ::::888a  8a ##::.   and/or modify it under the terms of the GNU Lesser *
 *  ::::::::888a88a[]:::   General Public License as published by the Free    *
 *::8:::::::::SUNDOGa8a::. Software Foundation, either version 2.1 of the     *
 *::::::::8::::888:Y8888:: License, or (at your option) any later version.    *
 *::::':::88::::888::Y88a::::::::::::...                                      *
 *::'::..    .   .....   ..   ...  .                                          *
 * This software is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.							      *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with software; if not, write to the Free Software Foundation, Inc.,  *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA                     *
 *                                                                            *
 * Clan, the Chunky Loop Analyzer                                             *
 * Written by Cedric Bastoul, Cedric.Bastoul@u-psud.fr                        *
 *                                                                            *
 ******************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <osl/int.h>
#include <osl/relation.h>
#include <clan/macros.h>
#include <clan/relation.h>


/*+****************************************************************************
 *                            Processing functions                            *
 ******************************************************************************/


/**
 * clan_relation_tag_array function:
 * this function tags a relation to explicit it is describing the array index of
 * a given array. This means using OpenScop representation that the very first
 * output dimension will correspond to the constraint dim = array_id. It updates
 * directly the relation provided as parameter. A new row and a new column are
 * inserted to the existing relation and the number of output dimensions is
 * incremented.
 * \param[in,out] relation The relation to tag.
 * \param[in]     array    The array number.
 */
void clan_relation_tag_array(osl_relation_p relation, int array) {
  if (relation == NULL)
    CLAN_error("relation cannot be array-tagged");

  osl_relation_insert_blank_row(relation, 0);
  osl_relation_insert_blank_column(relation, 1);
  osl_int_set_si(CLAN_PRECISION, relation->m[0], 1, -1);
  osl_int_set_si(CLAN_PRECISION, relation->m[0], relation->nb_columns-1,array);
  relation->nb_output_dims++;
}


/**
 * clan_relation_scattering function:
 * this function builds the scattering relation for the clan_statement_t
 * structures thanks to the parser current state of parser_scattering (rank)
 * and parser_depth (depth). The "rank" vector gives the "position" of the
 * statement for every loop depth (see Feautrier's demonstration of existence
 * of a schedule for any SCoP or CLooG's manual for original scattering
 * function to understand if necessary). This function just "expands" this
 * vector to a (2*n+1)-dimensional schedule for a statement at depth n and
 * returns it.
 * \param rank  The position of the statement at every loop depth.
 * \param depth The depth of the statement.
 */
osl_relation_p clan_relation_scattering(int * rank, int depth) {
  int i, j, nb_rows, nb_columns;
  osl_relation_p scattering;

  nb_rows    = (2 * depth + 1);
  nb_columns = (2 * depth + 1) + (depth) + (CLAN_MAX_PARAMETERS) + 2;
  scattering = osl_relation_pmalloc(CLAN_PRECISION, nb_rows, nb_columns);
  osl_relation_set_type(scattering, OSL_TYPE_SCATTERING);
  osl_relation_set_attributes(scattering, 2 * depth + 1, depth, 0,
                              CLAN_MAX_PARAMETERS);  
  
  // The output dimension identity
  for (i = 0; i < 2 * depth + 1; i++)
    osl_int_set_si(CLAN_PRECISION, scattering->m[i], i+1, -1);

  // The beta and alpha.
  j = 0;
  for (i = 0; i < depth; i++) {
    osl_int_set_si(CLAN_PRECISION, scattering->m[j], nb_columns-1, rank[i]);
    osl_int_set_si(CLAN_PRECISION, scattering->m[j+1], (2*depth+1)+i+1, 1);
    j += 2;
  }
  osl_int_set_si(CLAN_PRECISION,
                 scattering->m[nb_rows-1], nb_columns-1, rank[depth]);

  return scattering;
}


/**
 * clan_relation_outputize function:
 * this function adds an output dimension to each row of an input
 * relation (they are all supposed to correspond to an output relation
 * expression). It amounts at adding a negative identity after the first
 * column of the constraint matrix and at updating the number of output
 * dimensions.
 * \param[in,out] relation The relation where to add output dimensions.
 */
void clan_relation_outputize(osl_relation_p relation) {
  int i;
  osl_relation_p neg_identity = osl_relation_pmalloc(CLAN_PRECISION,
    relation->nb_rows, relation->nb_rows);
  
  for (i = 0; i < relation->nb_rows; i++)
    osl_int_set_si(CLAN_PRECISION, neg_identity->m[i], i, -1);

  osl_relation_insert_columns(relation, neg_identity, 1);
  osl_relation_free(neg_identity);
  relation->nb_output_dims = relation->nb_rows;
}


/**
 * clan_relation_new_output_vector function:
 * this function adds a new output dimension to a relation and a new
 * constraint corresponding to the new output dimension to an existing
 * relation. The new output dimension is added after the existing output
 * dimensions. It is supposed to be equal to the vector expression passed
 * as an argument. The input relation is direcly updated.
 * \param[in,out] relation The relation to add a new output dimension.
 * \param[in]     vector   The expression the new output dimension is equal to.
 */
void clan_relation_new_output_vector(osl_relation_p relation,
                                     osl_vector_p vector) {
  int i, new_col, new_row;

  if (relation == NULL)
    CLAN_error("cannot add a new output dimension to a NULL relation");
  else if (vector == NULL)
    CLAN_error("cannot add a NULL expression of an output dimension");
  else if (relation->precision != vector->precision)
    CLAN_error("incompatible precisions");

  if (relation->nb_output_dims == OSL_UNDEFINED)
    new_col = 1;
  else
    new_col = relation->nb_output_dims + 1;
  new_row = relation->nb_rows;

  if ((relation->nb_columns - (new_col - 1)) != vector->size)
    CLAN_error("incompatible sizes");
  else if (!osl_int_zero(vector->precision, vector->v, 0))
    CLAN_error("the output dimension expression should be an equality");

  // Prepare the space for the new output dimension and the vector.
  osl_relation_insert_blank_column(relation, new_col);
  osl_relation_insert_blank_row(relation, new_row);
  relation->nb_output_dims = new_col;

  // Insert the new output dimension.
  osl_int_set_si(relation->precision, relation->m[new_row], new_col, -1);
  for (i = 1; i < vector->size; i++)
    osl_int_assign(relation->precision,
                   relation->m[new_row], new_col + i, vector->v, i);
}


/**
 * clan_relation_new_output_scalar function:
 * this function adds a new output dimension to a relation and a new
 * constraint corresponding to the new output dimension to an existing
 * relation. The new output dimension is added after the existing output
 * dimensions. It is supposed to be equal to a scalar value passed
 * as an argument. The input relation is direcly updated.
 * \param[in,out] relation The relation to add a new output dimension.
 * \param[in]     scalar   The scalar the new output dimension is equal to.
 */
void clan_relation_new_output_scalar(osl_relation_p relation, int scalar) {
  int new_col, new_row;
  
  if (relation == NULL)
    CLAN_error("cannot add a new output dimension to a NULL relation");

  if (relation->nb_output_dims == OSL_UNDEFINED)
    new_col = 1;
  else
    new_col = relation->nb_output_dims + 1;
  new_row = relation->nb_rows;

  // Prepare the space for the new output dimension and the vector.
  osl_relation_insert_blank_column(relation, new_col);
  osl_relation_insert_blank_row(relation, new_row);
  relation->nb_output_dims = new_col;

  // Insert the new output dimension.
  osl_int_set_si(relation->precision, relation->m[new_row], new_col, -1);
  osl_int_set_si(relation->precision, relation->m[new_row],
                 relation->nb_columns - 1, scalar);
}


/**
 * clan_relation_compact function:
 * This function compacts a relation such that it uses the right number
 * of columns (during construction we used CLAN_MAX_DEPTH and
 * CLAN_MAX_PARAMETERS to define relation and vector sizes). It modifies
 * directly the relation provided as parameter.
 * \param relation      The relation to compact.
 * \param nb_iterators  The true number of iterators for this relation.
 * \param nb_parameters The true number of parameters in the SCoP.
 */
void clan_relation_compact(osl_relation_p relation, int nb_iterators, 
                           int nb_parameters) {
  int i, j, nb_columns, nb_output_dims, nb_input_dims;
  osl_relation_p compacted;

  if (relation == NULL)
    return;

  nb_output_dims = relation->nb_output_dims;
  nb_input_dims  = relation->nb_input_dims;

  nb_columns = nb_output_dims + nb_input_dims + nb_parameters + 2;
  compacted = osl_relation_pmalloc(CLAN_PRECISION,
                                   relation->nb_rows, nb_columns);

  for (i = 0; i < relation->nb_rows; i++) {
    /* We copy the equality/inequality tag, the output and the input
     * coefficients
     */
    for (j = 0; j <= nb_output_dims + nb_input_dims; j++)
      osl_int_assign(CLAN_PRECISION, compacted->m[i], j, relation->m[i], j);

    /* Then we copy the parameter coefficients */
    for (j = 0; j < nb_parameters; j++)
      osl_int_assign(CLAN_PRECISION,
          compacted->m[i], j + nb_output_dims + nb_input_dims + 1,
          relation->m[i], relation->nb_columns - CLAN_MAX_PARAMETERS -1 +j);

    /* Lastly the scalar coefficient */
    osl_int_assign(CLAN_PRECISION,
                   compacted->m[i], nb_columns - 1,
                   relation->m[i], relation->nb_columns - 1);
  }

  osl_relation_free_inside(relation);

  /* Replace the inside of relation */
  relation->nb_rows       = compacted->nb_rows;
  relation->nb_columns    = compacted->nb_columns;
  relation->m             = compacted->m;
  relation->nb_parameters = nb_parameters;

  /* Free the compacted "container" */
  free(compacted);
}


/**
 * clan_relation_greater function:
 * this function generates a relation corresponding to the equivalent
 * constraint set of two linear expressions sets involved in a
 * condition "greater (or equal) to". min and max are two linear expression
 * sets, e.g. (a, b, c) for min and (d, e) for max, where a, b, c, d and e
 * are linear expressions. This function creates the constraint set
 * corresponding to, e.g., min(a, b, c) > max(d, e), i.e.:
 * (a > d && a > e && b > d && b > e && c > d && c > e). It may also create
 * the constraint set corresponding to min(a, b, c) >= max(d, e).
 * The relations max and min are not using the OpenScop/PolyLib format:
 * each row corresponds to a linear expression as usual but the first
 * element is not devoted to the equality/inequality marker but to store
 * the value of the ceild or floord divisor. Hence, each row may correspond
 * to a ceil of a linear expression divided by an integer in min or
 * to a floor of a linear expression divided by an integer in max.
 * \param[in] min    Set of (ceild) linear expressions corresponding
 *                   to the minimum of the (ceild) linear expressions.
 * \param[in] max    Set of (floord) linear expressions corresponding
 *                   to the maximum of the (floord) linear expressions.
 * \param[in] strict 1 if the condition is min > max, 1 if it is min >= max.
 * \return A set of linear constraints corresponding to min > (or >=) max.
 */
osl_relation_p clan_relation_greater(osl_relation_p min, osl_relation_p max,
                                     int strict) {
  int imin, imax, j;
  int a, b;
  osl_relation_p r;
  osl_int_p b_min, a_max;

  if ((min == NULL) || (max == NULL) || (strict < 0) || (strict > 1) ||
      (min->nb_columns != max->nb_columns))
    CLAN_error("cannot compose relations");
  
  r = osl_relation_pmalloc(CLAN_PRECISION,
                           min->nb_rows * max->nb_rows, min->nb_columns);
  b_min = osl_int_malloc(CLAN_PRECISION); 
  a_max = osl_int_malloc(CLAN_PRECISION);
  
  // For each row of min
  for (imin = 0; imin < min->nb_rows; imin++) {
    // For each row of max
    // We have a couple min/a >= max/b to translate to b*min - a*max >= 0
    //      or a couple min/a > max/b  to translate to b*min - a*max - 1 >= 0
    // TODO: here a and b are > 0, this may be generalized to avoid a
    //       problem if the grammar is updated. Plus it's debatable to use
    //       b*min - a*max -a*b >= 0 in the second case. 
    
    // -1. Find a
    a = osl_int_get_si(CLAN_PRECISION, min->m[imin], 0);
    a = (a == 0) ? 1 : a;

    for (imax = 0; imax < max->nb_rows; imax++) {
      // -2. Find b
      b = osl_int_get_si(CLAN_PRECISION, max->m[imax], 0);
      b = (b == 0) ? 1 : b;

      // -3. Compute b*min - a*max to the new relation.
      for (j = 1; j < max->nb_columns; j++) {
        // -3.1. Compute b*min
        osl_int_mul_si(CLAN_PRECISION, b_min, 0, min->m[imin], j, b);
        
        // -3.2. Compute a*max
        osl_int_mul_si(CLAN_PRECISION, a_max, 0, max->m[imax], j, a);
        
        // -3.3. Compute b*min - a*max
        osl_int_sub(CLAN_PRECISION,
                    r->m[imin * max->nb_rows + imax], j, b_min, 0, a_max, 0);
      }

      // -4. Add -1 if the condition is min/a > max/b, add 0 otherwise.
      osl_int_add_si(CLAN_PRECISION,
                     r->m[imin * max->nb_rows + imax], max->nb_columns - 1,
                     r->m[imin * max->nb_rows + imax], max->nb_columns - 1,
                     -strict);
      // -5. Set the equality/inequality marker to inequality.
      osl_int_set_si(CLAN_PRECISION, r->m[imin * max->nb_rows + imax], 0, 1);
    }
  }

  osl_int_free(CLAN_PRECISION, b_min, 0);
  osl_int_free(CLAN_PRECISION, a_max, 0); 
  return r;
}
