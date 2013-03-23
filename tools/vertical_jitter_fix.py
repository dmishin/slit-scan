from PIL import Image
import numpy as np
from scipy.optimize import minimize_scalar
from numpy.linalg import norm
import math
from optparse import OptionParser
import sys

def Load(path):
    img = Image.open(path)
    arr = np.asarray(img)
    return arr.astype(np.float)

def fix_jitter( image_data, ref_range = None, 
                characteristic_time = 3, 
                shift_range = 15, 
                shift_tolerance=0.01 ):
    h, w, depth = image_data.shape
    assert(depth == 3)
    if ref_range is None:
        ref_range = (0, h)
    
    y = np.linspace(0, h-1, h)

    def get_col(col, chan):
        return image_data[:,col, chan]
    def set_col(col, chan, data):
        image_data[:,col, chan] = data

    def col_interpolator_lin(data):
        def interpolator(shift):
            return np.interp( y + shift, y, data )
        return interpolator

    def col_interpolator_dlin(data):
        def interpolator(shift):
            rblur = np.interp( y - shift, y, np.interp( y + shift, y, data ) )
            return np.clip( np.interp( y + shift, y, data*1.5-rblur*0.5 ), 0, 255 )
        return interpolator

    #Change this to use different interpolator
    use_interpolator = col_interpolator_lin

    def matching_function(base_col, col, order=1):
        rgb0 = [get_col(base_col, c) for c in xrange(depth)]
        rgb1 = [get_col(col, c) for c in xrange(depth)]

        interpolators = map(use_interpolator, rgb1)
        a,b = ref_range
        def match( shift ):
            s = 0.0
            for interpolator, reference_value in zip( interpolators, rgb0 ):
                s += norm(interpolator(shift)[a:b] - reference_value[a:b], order)
            return s
        return match

    #find optimal shift
    def find_shift( col, base_col ):
        func = matching_function( base_col, col )
        ans = minimize_scalar( func, 
                               bounds = (-shift_range, shift_range), 
                               method="bounded", tol=shift_tolerance)
        print "%04d of %d\r"%(col, w),
        sys.stdout.flush()
        return ans.x

    #Relative offsets between columns
    print "Pass 1 of 2"
    deltas = np.array([find_shift(i, i-1) for i in xrange(1,w)])
    #Offsets, relative to the first column
    print "Pass 2 of 2"
    abs_offsets = np.array([find_shift(i, 0) for i in xrange(1,w)])

    #Filtered offsets. 
    #Low-pass fitlering on absolute offsets + high-pass on relative.
    offsets = decay_cumsum(deltas, characteristic_time ) + low_pass( abs_offsets, characteristic_time )
    #Make offsets closer to integers
    offsets = integerize( offsets, characteristic_time*2 )

    #Generate the image
    for x in xrange(1, w):
        for c in xrange(depth):
            set_col( x, c, 
                     use_interpolator(get_col(x, c))(offsets[x-1]) )

def decay_cumsum( x, decay_time ):
    """Discrete integrator with decay (high-pass filter)"""
    k = 0.5 ** (1.0/decay_time)
    
    y = np.zeros(x.shape)
    s = 0.0
    for i in xrange(len(x)):
        s = s * k + x[i]
        y[i] = s
    return y

def low_pass( x, decay_time ):
    """Low-pass: discrete aperiodic filter"""
    k = 0.5 ** (1.0/decay_time)
    
    y = np.zeros(x.shape)
    y[0] = x[0]
    s = x[0]
    for i in xrange(1, len(x)):
        s = s * k + x[i]*(1-k)
        y[i] = s

    return y

def integerize( x, decay_time ):
    """Make sequence cling to the nearest integer"""
    xr = np.round(x)
    dx = x - xr
    return xr + dx - low_pass(dx,decay_time)

def main():
    usage = """%prog [options] input.png [output.png]
    Tries to remove vertical jitter from the image. 
    Useful for pos-processing slit-scan images, produced from the shaky videos.
    """
    parser = OptionParser(usage = usage)

    parser.add_option("-r", "--range", dest="range",
                      help="Use only specified range of pixeld to align image columns", metavar="START:END")

    parser.add_option("-t", "--damping-time", dest="damping_time",
                      type="float", default=3.0,
                      help="Characteristic time of a damping filter, in pixels (horizontal dimension)")

    (options, args) = parser.parse_args()
    
    if len(args) < 1:
        parser.error("Input file not specified")
    if len(args) > 2:
        parser.error("Too many arguments")
    if options.damping_time <= 0:
        parser.error("Damping time must be positive")
    
    if options.range:
        try:
            a, b = map(int, options.range.split(":"))
            rrange = (a,b)
            print "Using reference range: [%dpx .. %dpx]"%(a, b)
        except:
            parser.error("Range is incorrect")
    else:
        rrange = None

    if len(args) > 1:
        ofile = args[1]
    else:
        ofile = None

    ##########################
    img = Load(args[0])
    
    fix_jitter( img, 
                ref_range=rrange, 
                characteristic_time=options.damping_time )

    i1 = Image.fromarray(img.astype(np.uint8))
    if ofile is not None:
        i1.save(ofile)
    else:
        i1.show()

if __name__=="__main__":
    main()
