
from ctypes import windll, Structure, c_ulong, byref
import time
import matplotlib.pyplot as plt

# Code to find the mouse position.
class POINT(Structure):
    _fields_ = [("x", c_ulong), ("y", c_ulong)]

def queryMousePosition():
    pt = POINT()
    windll.user32.GetCursorPos(byref(pt))
    return (pt.x, pt.y)
# --------------------------------

class PIDLoop:
    def __init__(self, p, i, d):
        self.Kp = p     #Ki coefficient 
        self.Ki = i     #Ki coefficient
        self.Kd = d     #Kp coefficient

        self.P = 0
        self.I = 0
        self.D = 0
        
        self.output = 0

        self.prev_error = 0
        self.last_error = 0

        self.windup = 40
    
    def update(self, target, setpoint, dt):
        err = setpoint - target
        derr = err - self.prev_error
        
        self.P = self.Kp * err
        self.I += self.Ki * (err * dt)
        self.D = self.Kd * (derr / dt) if dt > 0 else 0

        if self.I > self.windup:
            self.I = self.windup
        if self.I < -self.windup:
            self.I = -self.windup

        self.prev_error = err
        self.output = self.P + self.I + self.D
        return self.output


pid = PIDLoop(1.2, 1, 0.001)
curr = 800
currs = [curr]
target = 400

for i in range(0, 50, 1):
    out = pid.update(target, curr, .005)
    print(str(out))
    curr -= out
    currs.append(curr)

plt.plot([i * 0.005 for i in range(0, 51, 1)], [400 for i in range (0, 51, 1)])
plt.plot([i * 0.005 for i in range(0, 51, 1)], currs)
plt.show()

























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












