
from ctypes import windll, Structure, c_ulong, byref

# Code to find the mouse position.
class POINT(Structure):
    _fields_ = [("x", c_ulong), ("y", c_ulong)]

def queryMousePosition():
    pt = POINT()
    windll.user32.GetCursorPos(byref(pt))
    return (pt.x, pt.y)
# --------------------------------






























# # Difference of point a and point b, returns a vector, v where v + a = b
# def pointDiff(a, b):
#     return (b[0] - a[0], b[1] - a[1])


# #print(queryMousePosition())
# #print(pointDiff(head, queryMousePosition()))

# def correctionX(actual, target, prev_error, integral, dt):
#     # dt = time step (ms).
#     kp = 5.0 # Proportional gain.
#     ki = 1.2 # Intergral gain.
#     kd = 0.4 # Derivative gain.

#     err = target[0] - actual[0] #error
#     integral = integral + err*dt       #integral

#     print("actual: " + str(actual[0]), "target: " + str(target[0]), "error: " + str(err), "dt: " + str(dt))

#     der = (err - prev_error) / dt      #derivative
#     prev_error = err                   #setting past error

#     print(str(kp*err), str(ki*integral), str(kd*der))

#     return prev_error, integral, (kp*err + ki*integral + kd*der) #output calculation


# integral = 0
# prev_error = 0
# dt = 1
# head = (960, 540)
# outputs = []
# mouse = (0, 0)

# for i in range(0, 20, 1):
#     prev_error, integral, output = correctionX(mouse, head, prev_error, integral, dt)
#     mouse = (mouse[0] + output, 0)
#     outputs.append(output)
#     print("x correction: " + str(output), ", mouse_x: " + str(mouse[0]), "prev_error:" + str(prev_error), "integral: " + str(integral))
#     print("\n")












