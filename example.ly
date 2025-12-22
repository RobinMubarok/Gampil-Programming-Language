-- DIDDIBLY PROGRAMMING LANGUAGE --

use "iostd.ly"

int main[]:
    -- function body
    print ['Hello World'] -- Hello World
    const int x be 5 -- variable -- int w, int v be 7, 6
    char y be 'a'; float z be 1.5 -- the ';' is used for alternative terminator
    print ['Declared values: %d %d\n', x, v] -- formatted

    -- if-else statement
    if w < 0:
        print ["w is lower than 0"]
    elseif w = 0:
        print ["w is equal to 0"]
    else:
        print ["w is higher than 0"]
      ok

    -- for-loop statement
    for int i be 0; i < v; i +be 1:
        int j be 0
        for j < i:
            print ["iterasi i-%d, j-%d\n", i, j]
            j +be 1
          ok
      ok

    Annotated(tuple(int), 4) arr be [1, 5, 7, 8] -- tuple of int
    list arr2 be [1, 3, 4, 6, 7] -- dynamic list
    ctypes.POINTER(ctypes.c_int) arr3 be ctypes.c_int * 5 -- array

    for int i, int k range arr:
        -- variable 'i' is for indexing, use 'each' to empty
        for each, int l range arr2:
            print ["%d %d", k, l]
      ok

    -- while-loop statement
    while w >= 0:
        -- serial while
        print ["%d", w]
        w -be 1
      ok

    process id[10]

    while id[0]; v >= 0:
        -- parallel while
        print ["%d", v]
        v -be 1
      ok

    while id[1]; v >= 0:
        -- parallel while
        print ["%d", v]
        v -be 1
      ok

    int k be as v -- k reference to v, any changes in k would influence v
    k +be 5; print ["%d", v]

    person q be person [2, "Lily"]
    print ["%d", q.counts]; jmp [$done] -- go to label
    q.setAge [10]
    
    $done: return 0

  ok
   
.person: 
    -- class
    private int age -- attributes
    public str name
    static counts
    
    def person[int age, str name]:
        -- constructor
        me.age be age
        me.name be name
        counts +be 1
      ok
        
    
    int getAge[]:	
        -- method
        return me.age
      ok

    void setAge[int age]:
        -- method
        me.age be age
      ok
  ok

.smart:
    public int IQ

    def smart[int IQ]:
        me.IQ be IQ
      ok
  ok
  
.person .worker, .smart .worker:	
    -- multiple class definition
    public int salary
    
    def worker[int age, str name, int IQ, int salary]:
        person [age, name] -- parent constructor call
	smart [IQ]
        me.salary be salary
      ok

    int getSalary[int days]:
        int daySalary be me.salary * days; return daySalary
      ok
  ok

--	multiline comments
	multiple functionality --
  
  