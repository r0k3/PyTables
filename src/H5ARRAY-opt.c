#include "H5ARRAY-opt.h"

#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------
 * Function: H5ARRAYOread_readSlice
 *
 * Purpose: Read records from an opened Array
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: May 27, 2004
 *
 * Comments:
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOread_readSlice( hid_t dataset_id,
			       hid_t space_id,
			       hid_t type_id,
			       hsize_t irow,
			       hsize_t start,
			       hsize_t stop,
			       void *data )
{
 hid_t    mem_space_id;
 hsize_t  count[2];
 int      rank = 2;
 hsize_t  offset[2];
 hsize_t  stride[2] = {1, 1};

 count[0] = 1;
 count[1] = stop - start;
 offset[0] = irow;
 offset[1] = start;

 /* Create a memory dataspace handle */
 if ( (mem_space_id = H5Screate_simple( rank, count, NULL )) < 0 )
   goto out;

 /* Define a hyperslab in the dataset of the size of the records */
 if ( H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, stride, count, NULL) < 0 )
   goto out;

 /* Read */
 if ( H5Dread( dataset_id, type_id, mem_space_id, space_id, H5P_DEFAULT, data ) < 0 )
   goto out;

 /* Terminate access to the memory dataspace */
 if ( H5Sclose( mem_space_id ) < 0 )
   goto out;

return 0;

out:
 H5Dclose( dataset_id );
 return -1;

}


herr_t H5ARRAYOread_index_sparse( hid_t dataset_id,
				  hid_t space_id,
				  hid_t mem_type_id,
				  hsize_t ncoords,
				  void *coords,
				  void *data )
{
 hid_t    mem_space_id;
 hsize_t  mem_size[1];

 if ( H5Sselect_elements(space_id, H5S_SELECT_SET, (size_t)ncoords,
			 (const hsize_t **)coords) < 0 )
  goto out;

 /* Create a memory dataspace handle */
 mem_size[0] = ncoords;
 if ( (mem_space_id = H5Screate_simple(1, mem_size, NULL)) < 0 )
  goto out;

 if ( H5Dread(dataset_id, mem_type_id, mem_space_id, space_id,
	      H5P_DEFAULT, data) < 0 )
  goto out;

 /* Terminate access to the memory dataspace */
 if ( H5Sclose(mem_space_id) < 0 )
  goto out;

return 0;

out:
 H5Dclose(dataset_id);
 return -1;

}


/*-------------------------------------------------------------------------
 * Function: H5ARRAYOopen_readSortedSlice
 *
 * Purpose: Prepare an array to be read incrementally
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: Aug 11, 2005
 *
 * Comments:
 *   - The H5ARRAYOopen_readSlice, H5ARRAYOread_readSlice and
 *     H5ARRAYOclose_readSlice are intended to read Indexes slices only!
 *     F. Altet 2005-08-11
 *
 * Modifications:
 *   - Modified to cache the mem_space_id as well.
 *     F. Altet 2005-08-11
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOopen_readSortedSlice( hid_t *dataset_id,
				     hid_t *space_id,
				     hid_t *mem_space_id,
				     hid_t *type_id,
				     hid_t loc_id,
				     const char *dset_name,
				     hsize_t count)

{
 int      rank = 2;
 hsize_t  count2[2] = {1, count};

 /* Open the dataset. */
 if ( (*dataset_id = H5Dopen( loc_id, dset_name )) < 0 )
  return -1;

 /* Get the datatype */
 if ( (*type_id = H5Dget_type(*dataset_id)) < 0 )
     return -1;

  /* Get the dataspace handle */
 if ( (*space_id = H5Dget_space(*dataset_id )) < 0 )
  goto out;

 /* Create a memory dataspace handle */
 if ( (*mem_space_id = H5Screate_simple(rank, count2, NULL)) < 0 )
   goto out;

 return 0;

out:
 H5Dclose( *dataset_id );
 return -1;

}

/*-------------------------------------------------------------------------
 * Function: H5ARRAYOread_readSortedSlice
 *
 * Purpose: Read records from an opened Array
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: Aug 11, 2005
 *
 * Comments:
 *
 * Modifications:
 *   - Modified to cache the mem_space_id as well.
 *     F. Altet 2005-08-11
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOread_readSortedSlice( hid_t dataset_id,
				     hid_t space_id,
				     hid_t mem_space_id,
				     hid_t type_id,
				     hsize_t irow,
				     hsize_t start,
				     hsize_t stop,
				     void *data )
{
 hsize_t  count[2] = {1, stop-start};
 hssize_t offset[2] = {irow, start};
 hsize_t  stride[2] = {1, 1};

 /* Define a hyperslab in the dataset of the size of the records */
 if ( H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, stride, count, NULL) < 0 )
   goto out;

 /* Read */
 if ( H5Dread( dataset_id, type_id, mem_space_id, space_id, H5P_DEFAULT, data ) < 0 )
   goto out;

return 0;

out:
 H5Dclose( dataset_id );
 return -1;

}


/*-------------------------------------------------------------------------
 * Function: H5ARRAYOread_readBoundsSlice
 *
 * Purpose: Read records from an opened Array
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: Aug 19, 2005
 *
 * Comments:  This is exactly the same as H5ARRAYOread_readSortedSlice,
 *    but I just want to distinguish the calls in profiles.
 *
 * Modifications:
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOread_readBoundsSlice( hid_t dataset_id,
				     hid_t space_id,
				     hid_t mem_space_id,
				     hid_t type_id,
				     hsize_t irow,
				     hsize_t start,
				     hsize_t stop,
				     void *data )
{
 hsize_t  count[2] = {1, stop-start};
 hssize_t offset[2] = {irow, start};
 hsize_t  stride[2] = {1, 1};

 /* Define a hyperslab in the dataset of the size of the records */
 if ( H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, stride, count, NULL) < 0 )
   goto out;

 /* Read */
 if ( H5Dread( dataset_id, type_id, mem_space_id, space_id, H5P_DEFAULT, data ) < 0 )
   goto out;

return 0;

out:
 H5Dclose( dataset_id );
 return -1;

}


/*-------------------------------------------------------------------------
 * Function: H5ARRAYOclose_readSortedSlice
 *
 * Purpose: Close a table that has been opened for reading
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: Aug 11, 2005
 *
 * Comments:
 *
 * Modifications:
 *   - Modified to cache the mem_space_id as well.
 *     F. Altet 2005-08-11
 *
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOclose_readSortedSlice(hid_t dataset_id,
				     hid_t space_id,
				     hid_t mem_space_id,
				     hid_t type_id)
{

 /* Terminate access to the dataspace */
 if ( H5Sclose( space_id ) < 0 )
  goto out;

 /* Terminate access to the memory dataspace */
 if ( H5Sclose( mem_space_id ) < 0 )
   goto out;

 /* Close the type */
 if ( H5Tclose( type_id))
   return -1;

 /* End access to the dataset and release resources used by it. */
 if ( H5Dclose( dataset_id ) )
  return -1;

return 0;

out:
 H5Dclose( dataset_id );
 return -1;

}

/*-------------------------------------------------------------------------
 * Function: H5ARRAYreadSliceLR
 *
 * Purpose: Reads a slice of LR index cache from disk.
 *
 * Return: Success: 0, Failure: -1
 *
 * Programmer: Francesc Altet, faltet@carabos.com
 *
 * Date: August 17, 2005
 *
 *-------------------------------------------------------------------------
 */

herr_t H5ARRAYOreadSliceLR(hid_t dataset_id,
			   hsize_t start,
			   hsize_t stop,
			   void *data)
{
 hid_t    space_id;
 hid_t    mem_space_id;
 hid_t    type_id;
 hsize_t  dims[1] = {1};
 hsize_t  count[1] = {stop - start};
 hsize_t  stride[1] = {1};
 hssize_t offset[1] = {start};
 int      rank;
 int      i;

 /* Get the datatype */
 if ( (type_id = H5Dget_type(dataset_id)) < 0 )
     return -1;

  /* Get the dataspace handle */
 if ( (space_id = H5Dget_space(dataset_id)) < 0 )
  goto out;

 /* Define a hyperslab in the dataset of the size of the records */
 if ( H5Sselect_hyperslab(space_id, H5S_SELECT_SET, offset, stride, count, NULL) < 0 )
   goto out;

 /* Create a memory dataspace handle */
 if ( (mem_space_id = H5Screate_simple(1, count, NULL)) < 0 )
   goto out;

 /* Read */
 if ( H5Dread( dataset_id, type_id, mem_space_id, space_id, H5P_DEFAULT, data ) < 0 )
   goto out;

 /* Release resources */

 /* Terminate access to the memory dataspace */
 if ( H5Sclose( mem_space_id ) < 0 )
   goto out;

 /* Terminate access to the dataspace */
 if ( H5Sclose( space_id ) < 0 )
   goto out;

 /* Close the type */
 if ( H5Tclose(type_id))
   return -1;

 return 0;

out:
 H5Dclose( dataset_id );
 return -1;

}
