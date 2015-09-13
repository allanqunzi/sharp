class Animal(object):
    def __init__(self,name,legs):
        self.name = name
        self.legs = legs

    def getLegs(self):
        return "{0} has {1} legs".format(self.name, self.legs)

    def says(self):
        return "I am an unknown animal"

class Dog(Animal): # <Dog inherits from Animal here (all methods as well)

    def __init__(self, name, legs):
        super(Dog, self).__init__(name, legs)


    def says(self): # <Called instead of Animal says method
        return "I am a dog named {0}".format(self.name)

    def somethingOnlyADogCanDo(self):
        return "be loyal"

formless = Animal("Animal", 0)
rover = Dog("Rover", 4) #<calls initialization method from animal

print(formless.says()) # <calls animal say method

print(rover.says()) #<calls Dog says method
print(rover.getLegs()) #<calls getLegs method from animal class

class BaseTrader(object):

    def __init__(self, wl = [], cores = 2):
        self.wl = wl
        self.cores = cores
        self._dict = {}
        self._spread_wl()
        self.sts = False

    def _spread_wl(self):
        num_p = (self.cores - 1) # number of processes
        num_s = len(self.wl)     # number of symbols
        a = num_s/num_p
        b = num_s%num_p
        cur = 0
        for i in range(num_p):
            self._dict[i+1] = [self.wl[cur]]
            cur += 1
            for j in range(cur, cur+a-1):
                self._dict[i+1].append(self.wl[j])
            cur += a-1
            if b > 0:
                self._dict[i+1].append(self.wl[cur])
                b -= 1
                cur += 1
        print "cur =",cur
        assert num_s == cur, "error in _spread_wl"

    def status(self):
        return self.sts


trader = BaseTrader(["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "e" ], 11)

if not trader.status():
    print '*********'