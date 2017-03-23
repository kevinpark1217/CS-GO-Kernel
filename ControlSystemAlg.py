
from ctypes import windll, Structure, c_ulong, byref

# Code to find the mouse position.
class POINT(Structure):
    _fields_ = [("x", c_ulong), ("y", c_ulong)]

def queryMousePosition():
    pt = POINT()
    windll.user32.GetCursorPos(byref(pt))
    return (pt.x, pt.y)
# --------------------------------

# Difference of point a and point b, returns a vector, v where v + a = b
def pointDiff(a, b):
    return (a[0] - b[0], a[1] - b[1])


head = (960, 540)

print(queryMousePosition())
print(pointDiff(head, queryMousePosition()))


prev_error = 0
integral = 0

def correctionX(actual, target, prev_error, integral, dt):
    # dt = time step (ms).
    kp = 0.0 # Proportional gain.
    ki = 0.0 # Intergral gain.
    kd = 0.0 # Derivative gain.
    
    err = pointDiff(actual, target)[0]
    in = integral + err*dt
    der = (err - prev_error) / dt
    prev_error = err
    return out = kp*err + ki*in + kd*der










