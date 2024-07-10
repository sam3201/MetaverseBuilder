## TYPES_SCRIPT

Syntax

KEYWORDS
TYPE
    CONSTRUCT 
    SELF
    PROP
    ATTR

COLOR, POS 

Logic
Opening Bracket {
Closing Bracket }
Comma ,
Colon :
    - used to delimit the type of a property
Equal =
    - used for intialization of variables 

EX:
Limited to None polymorphism

TYPE (SNAKE_HEAD: SELF) {
    SELF: ENTITY(SELF, CHAR{0}, POS{0, 0})
}

	#Initial compile returns a list named after the TYPE or SELF 
TYPE (ROBOT: SELF) {
	CONSTRUCT(SELF, CHAR: char, POS: pos) { //char(s) and pos(s) are required
        SELF: ENTITY(SELF, char, pos) 
    #Initial compile returns a list called SELF which contains the ENTITY object itself 
    }
	PROP
		COLOR{0, 255, 0} 
		CHAR{O}
	PROP
        allowed_spawns(SELF: SELF)
            POS{0, 0}
    
    #Initial compile returns a list aggregating the PROPS values 

	ATTR:
		new_part(SELF, NUM: len, CHAR: char, POS: pos) {
        FOR INT: i IN 0 TO len { 
            IF i != 0 THEN
                SELF[i]: ENTITY(NONALIVE, char, pos) 
        }
    }
        #Initial compile returns a list of the ATTR methods 


	destruct(SELF: SELF) {
        #the compiler will call entity_free() 

    }
}
}


