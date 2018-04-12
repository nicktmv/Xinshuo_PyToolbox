# Author: Xinshuo Weng
# email: xinshuo.weng@gmail.com
from PIL import Image
import numpy as np

import init_paths
from image_processing import rgb2hsv
from xinshuo_miscellaneous import isgrayimage
from xinshuo_visualization import visualize_image

def test_rgb2hsv():
	image_path = 'lena.jpg'

	# test for rgb pil image
	img = Image.open(image_path).convert('RGB')
	visualize_image(img, vis=True)
	hsv_img = rgb2hsv(img)
	visualize_image(hsv_img, vis=True)

	# test for rgb numpy image
	img = np.array(Image.open(image_path).convert('RGB'))
	visualize_image(img, vis=True)
	hsv_img = rgb2hsv(img)
	visualize_image(hsv_img, vis=True)

	# test for rgb numpy float image
	img = np.array(Image.open(image_path).convert('RGB')).astype('float32') / 255.
	visualize_image(img, vis=True)
	hsv_img = rgb2hsv(img)
	visualize_image(hsv_img, vis=True)

	# test for gray pil image
	img = Image.open(image_path).convert('L')
	visualize_image(img, vis=True)
	try:
		hsv_img = rgb2hsv(img)
		visualize_image(hsv_img, vis=True)
	except AssertionError:
		print('the function does not work when the input is not a rgb image')

	print('\n\nDONE! SUCCESSFUL!!\n')

if __name__ == '__main__':
	test_rgb2hsv()