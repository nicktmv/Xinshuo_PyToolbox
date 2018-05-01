# Author: Xinshuo Weng
# email: xinshuo.weng@gmail.com

# this file includes functions checking the datatype and equality of input variables
import os, numpy as np
from PIL import Image

############################################################# basic and customized datatype
# note:
#       the tuple with length of 1 is equivalent to just the single element, it is not a tuple anymore
#       the boolean value True and False are the scalar value 1 and 0 respectively
def isstring(string_test):
	return isinstance(string_test, basestring)

def islist(list_test):
    return isinstance(list_test, list)

def islogical(logical_test):
    return isinstance(logical_test, bool)

def isnparray(nparray_test):
    return isinstance(nparray_test, np.ndarray)

def istuple(tuple_test):
    return isinstance(tuple_test, tuple)

def isfunction(func_test):
    return callable(func_test)

def isdict(dict_test):
    return isinstance(dict_test, dict)

def isext(ext_test):
    '''
    check if it is an extension
    '''
    return isstring(ext_test) and ext_test[0] == '.' and len(ext_test) > 1 and ext_test.count('.') == 1

def isrange(range_test):
    '''
    check if it is a data range
    '''
    return is2dpts(range_test)

def isscalar(scalar_test):
    try: return isinteger(scalar_test) or isfloat(scalar_test)
    except TypeError: return False

############################################################# value
def isinteger(integer_test):
    if isnparray(integer_test): return False
    try: return isinstance(integer_test, int) or int(integer_test) == integer_test
    except ValueError: return False
    except TypeError: return False

def isfloat(float_test):
    return isinstance(float_test, float)

def ispositiveinteger(integer_test):
    return isinteger(integer_test) and integer_test > 0

def isnonnegativeinteger(integer_test):
    return isinteger(integer_test) and integer_test >= 0

def isuintnparray(nparray_test):
    return isnparray(nparray_test) and nparray_test.dtype == 'uint8'

def isfloatnparray(nparray_test):
    return isnparray(nparray_test) and nparray_test.dtype == 'float32'

def isnannparray(nparray_test):
    return isnparray(nparray_test) and np.isnan(nparray_test).any()

############################################################# list
def islistoflist(list_test):
    if not islist(list_test): return False
    if all(islist(tmp) for tmp in list_test): return True
    else: return False

def islistofdict(list_test):
    if not islist(list_test): return False
    if all(isdict(tmp) for tmp in list_test): return True
    else: return False  

def islistofscalar(list_test):
    if not islist(list_test): return False
    if all(isscalar(tmp) for tmp in list_test): return True
    else: return False  

def islistofpositiveinteger(list_test):
    if not islist(list_test): return False
    if all(ispositiveinteger(tmp) for tmp in list_test): return True
    else: return False  

def islistofnonnegativeinteger(list_test):
    if not islist(list_test): return False
    if all(isnonnegativeinteger(tmp) for tmp in list_test): return True
    else: return False  

############################################################# geometry
def is2dpts(pts_test):
    '''
    2d point coordinate
    numpy array or list or tuple with 2 elements
    '''
    return (isnparray(pts_test) or islist(pts_test) or istuple(pts_test)) and np.array(pts_test).size == 2

def is2dhomopts(pts_test):
    '''
    2d homogeneous point coordinate
    numpy array or list or tuple with 3 elements
    '''
    return is3dpts(pts_test)

def is2dptsarray(pts_test):
    '''
    numpy array with [2, N], N >= 0
    '''
    return isnparray(pts_test) and pts_test.shape[0] == 2 and len(pts_test.shape) == 2 and pts_test.shape[1] >= 0

def is2dptsarray_occlusion(pts_test):
    '''
    numpy array with [3, N], N >= 0. The third row represents occlusion, which contains only 1 or 0 or -1
    '''
    return is3dptsarray(pts_test) and (np.logical_or(np.logical_or(pts_test[2, :] == 0, pts_test[2, :] == 1), pts_test[2, :] == -1)).all()

def is2dptsarray_confidence(pts_test):
    '''
    numpy array with [3, N], N >= 0, the third row represents confidence, which contains a floating value bwtween [-1, 2] (as sometimes is 1.01 or -0.01)
    '''
    return is3dptsarray(pts_test) and (pts_test[2, :] >= -1).all() and (pts_test[2, :] <= 2).all()

def is2dptsarray_homogeneous(pts_test):
    '''
    numpy array with [2, N], N >= 0
    '''
    return is3dptsarray(pts_test)

def is3dpts(pts_test):
    '''
    numpy array or list or tuple with 3 elements
    '''
    return (isnparray(pts_test) or islist(pts_test) or istuple(pts_test)) and np.array(pts_test).size == 3

def is3dhomopts(pts_test):
    '''
    numpy array or list or tuple with 3 elements
    '''
    return (isnparray(pts_test) or islist(pts_test) or istuple(pts_test)) and np.array(pts_test).size == 4

def is3dptsarray(pts_test):
    '''
    numpy array with [3, N], N >= 0
    '''
    return isnparray(pts_test) and pts_test.shape[0] == 3 and len(pts_test.shape) == 2 and pts_test.shape[1] >= 0                   

def is2dhomoline(line_test):
    '''
    numpy array or list or tuple with 3 elements
    '''
    return is2dhomopts(line_test)

def islinesarray(line_test):
    return isnparray(line_test) and line_test.shape[0] == 4 and len(line_test.shape) == 2 and line_test.shape[1] >= 0               # 4 x N

def isbbox(bbox_test):
    return isnparray(bbox_test) and len(bbox_test.shape) == 2 and bbox_test.shape[0] > 0 and bbox_test.shape[1] == 4

def iscenterbbox(bbox_test):
    return isnparray(bbox_test) and len(bbox_test.shape) == 2 and bbox_test.shape[0] > 0 and (bbox_test.shape[1] == 4 or bbox_test.shape[1] == 2)

############################################################# image
def isimsize(size_test):
    return is2dpts(size_test)

def isnpimage_dimension(image_test):
    return isnparray(image_test) and ((image_test.ndim == 3 and (image_test.shape[2] == 3 or image_test.shape[2] == 1)) or image_test.ndim == 2)

def ispilimage(image_test):
    return isinstance(image_test, Image.Image)

def iscolorimage_dimension(image_test):
    return isnparray(image_test) and image_test.ndim == 3 and image_test.shape[2] == 3

def iscolorimage(image_test, debug=False):
    # if debug:
        # print 'is numpy array when testing color image? ', 
    if not isnparray(image_test):
        # if debug:
            # print 'No'
        return False
    # else:
        # if debug:
            # print 'Yes'

    # if debug:
        # print 'is dimension correct when testing color image? ', 
    shape_check = (image_test.ndim == 3 and (image_test.shape[2] == 3 or image_test.shape[2] == 4))     # rgb or rgba
    if shape_check:
        # if debug:
            # print 'Yes'
        return True 
    else:
        # if debug:
            # print 'No'
        return False

def isgrayimage_dimension(image_test):
    return isnparray(image_test) and (image_test.ndim == 2 or (image_test.ndim == 3 and image_test.shape[2] == 1))

def isgrayimage(image_test, debug=False):
    # if debug:
        # print 'is numpy array when testing grayscale image? ', 
    if not isnparray(image_test):
        # if debug:
            # print 'No'
        return False
    # else:
        # if debug:
            # print 'Yes'

    # if debug:
        # print 'is dimension correct when testing grayscale image? ', 
    shape_check = (image_test.ndim == 2 or (image_test.ndim == 3 and image_test.shape[2] == 1))
    if shape_check:
        # if debug:
            # print 'Yes'
        return True 
    else:
        # if debug:
            # print 'No'
        return False

def isuintimage(image_test, debug=False):
    # if debug:
        # print 'is shape correct when testing uint8 image? ', 
    if not (isgrayimage(image_test, debug=debug) or iscolorimage(image_test, debug=debug)):
        # if debug:
            # print 'No, shape is not correct'
        return False
    # else:
        # if debug:
            # print 'Yes, shape is correct'

    # if debug:
        # print 'is type correct when testing uint8 image? ', 
    if not image_test.dtype == 'uint8':
        # if debug:    
            # print 'No'
        return False
    # else:
        # if debug:
            # print 'Yes'

    # if debug:
        # print 'is value inside array correct when testing uint8 image? ', 
    item_check_le = (image_test <= 255)
    item_check_se = (image_test >= 0)
    if item_check_le.all() and item_check_se.all(): return True
    else: return False

def isfloatimage(image_test, debug=False):
    # if debug:
        # print 'is shape correct when testing float32 image? ', 
    if not (isgrayimage(image_test, debug=debug) or iscolorimage(image_test, debug=debug)): return False
    # else:
        # if debug:
            # print 'Yes, shape is correct'

    # if debug:
        # print 'is type correct when testing float32 image? ', 
    if not image_test.dtype == 'float32': return False
    # else:
        # if debug:
            # print 'Yes'

    # if debug:
        # print 'is value inside array correct ([0, 1]) when testing float32 image? ', 
    item_check_le = (image_test <= 1.0)
    item_check_se = (image_test >= 0.0)
    if item_check_le.all() and item_check_se.all(): return True
    else: return False

def isnpimage(image_test, debug=False):
    return isfloatimage(image_test, debug=debug) or isuintimage(image_test, debug=debug)

def isimage(image_test, debug=False):
    return isnpimage(image_test, debug=debug) or ispilimage(image_test)

def isscaledimage(image_test):
    if not isimage(image_test): return False
    max_value = np.max(image_test)
    min_value = np.min(image_test)
    assert min_value >= 0, 'image value is not correct'
    assert max_value >= 0, 'image value is not correct' 
    if max_value > 1 and max_value < 255:
        print('input image is raw image in [0, 255]')
        return False
    elif max_value <= 1: return True
    else: assert False, 'Unknown error'

############################################################# path 
def safepath(pathname, debug=True):
    '''
    convert path to a normal representation
    '''
    if debug: assert is_path_valid(pathname), 'path is not valid: %s' % pathname
    return os.path.normpath(pathname)

def is_path_valid(pathname):
    '''
    `True` if the passed pathname is a valid pathname for the current OS;
    `False` otherwise.
    '''
    # If this pathname is either not a string or is but is empty, this pathname
    # is invalid.
    try: 
        if not isstring(pathname) or not pathname: return False
    except TypeError: return False
    else: return True

def is_path_creatable(pathname):
    '''
    `True` if the current user has sufficient permissions to create the passed
    pathname; `False` otherwise.

    For folder, it needs the previous level of folder existing
    for file, it needs the folder existing
    '''
    if not is_path_valid(pathname): return False
    pathname = safepath(pathname)
    pathname = os.path.dirname(os.path.abspath(pathname))
    
    # recursively to find the root existing
    while not is_path_exists(pathname):     
        pathname_new = os.path.dirname(os.path.abspath(pathname))
        if pathname_new == pathname: return False
        pathname = pathname_new
    return os.access(pathname, os.W_OK)

def is_path_exists_or_creatable(pathname):
    '''
    this function is to justify is given path existing or creatable
    '''
    try: return is_path_valid(pathname) and (os.path.exists(pathname) or is_path_creatable(pathname))
    except OSError: return False

def is_path_exists(pathname):
    '''
    this function is to justify is given path existing or not
    '''
    try: return is_path_valid(pathname) and os.path.exists(pathname)
    except OSError: return False

def isfile(pathname):
    if is_path_valid(pathname):
        pathname = safepath(pathname)
        name = os.path.splitext(os.path.basename(pathname))[0]
        ext = os.path.splitext(pathname)[1]
        return len(name) > 0 and len(ext) > 0
    else: return False;

def isfolder(pathname):
    if is_path_valid(pathname):
        pathname = safepath(pathname)
        if pathname == './': return True
        name = os.path.splitext(os.path.basename(pathname))[0]
        ext = os.path.splitext(pathname)[1]
        return len(name) > 0 and len(ext) == 0
    else: return False

############################################################# equality check
def CHECK_EQ_LIST_SELF(input_list, debug=True):
	'''
	check all elements in a list are equal
	'''
	if debug: assert islist(input_list), 'input is not a list'
	return input_list[1:] == input_list[:-1]

def CHECK_EQ_DICT(input_dict1, input_dict2, debug=True):
    '''
    check all elements in a list are equal
    '''
    if debug:
        assert isdict(input_dict1) and isdict(input_dict2), 'input is not a dictionary'
        assert len(input_dict1) == len(input_dict2), 'length of input dictionary is not equal'

    for key, value in input_dict1.items():
        if input_dict2.has_key(key) and input_dict2[key] == value:
            continue
        else: return False
    return True

def CHECK_EQ_LIST_ORDERED(input_list1, input_list2, debug=True):
    '''
    check two lists are equal in ordered way
    '''
    if debug: assert islist(input_list1) and islist(input_list2), 'input lists are not correct'
    return input_list1 == input_list2

def CHECK_EQ_LIST_UNORDERED(input_list1, input_list2, debug=True):
    '''
    check two lists are equal in ordered way
    '''
    if debug: assert islist(input_list1) and islist(input_list2), 'input lists are not correct'
    return set(input_list1) == set(input_list2)

def CHECK_EQ_NUMPY(np_data1, np_data2, debug=True):
    '''
    check two numpy data are equal
    '''
    if debug:
        assert isnparray(np_data1) and isnparray(np_data2), 'the input numpy data is not correct'
        assert np_data2.shape == np_data1.shape, 'the shapes of two data blob are not equal'

    return np.all(np_data1 == np_data2)