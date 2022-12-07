class Params:
    def __init__(self):
        self.nominal_a = -0.14/0.08
        self.C0 = 0.0193
        self.lbda = 530e-09
        self.f0 = 100e-03
        self.fe = 40e-03
        self.SLMpitch = 3.74e-06
        self.W = 4.0

        self.slmWidth = 4000
        self.slmHeight = 2464

    def set_W(self, W):
        self.W = W
