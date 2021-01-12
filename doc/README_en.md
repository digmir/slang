# SLANG 
***
 A simple programming language. 

#### Keywords:
    .#?~@()<>[]{},;=\"'!^+-*/\\&|


#### Grammar:  
##### 1. Module
    A source file is a module, and a module can have multiple nodes.

##### 2. Data
    Constant: "value"
    Variable: Variable name

##### 3. Null
    Null: -

##### 4. Node
    .Node name(variable, variable, ...)(variable, variable, ...)(variable, variable, ...)  
    {  
        /* code segment */  
    }  
* The node name must start with __`.`__
* All variable lists can be transferred to parameters.
* The first variable list will be released when node return.
* The second variable list is cache,will not invalid when the node return.
* The third and later variable list has file persistence, when the program is restarted, the content still exists.  

##### 5. Assignment statement  
    
    variable = -;                       /* Null assigned */  
    variable = data;                    /* Data assignment */  
    variable = ?data;                   /* Expression assignment */  
    variable = *data;                   /* Format assignment */  
    variable = variable[data];          /* Take variable field value */  
    variable = variable["begin~end"];   /* Take multiple field values of variable */  
    variable = #variable;               /* Take variable length */  
    

* End with __`;`__.
    
##### 6. Control Flow - If statement  
    ?data{  /* The data is "1"? */  
        /* code segment */  
    }~data{ /* Otherwise, the data is "1" */  
        /* code segment */  
    }~{     /* Otherwise */  
        /* code segment */  
    }  
    
* The code segment must be in __`{ }`__.
    
##### 7. Loop statement  
    @data{  /* Loop execution until the data is not "1" */  
        /* code segment */  
    }  
    
* The code segment must be in __`{ }`__.

##### 8. Traversal statement   
    name,value @ data{  
        /* code segment */  
    }  
    
    name @ data{  
        /* code segment */  
    }  
    
    name,value,separator @ string{  
        /* code segment */  
    }  
* The code segment must be in __`{ }`__.
    
##### 9. Node return   
    < data, data, ... >; /* Can return multiple variables, constants, or null */

##### 10. Node call   
    ( data, ... ) > .Node name >  ( variable, ... );  //synchronous  
    ( data, ... ) > .Node name >> ( variable, ... );  //asynchronous  

__Or__

    ( data, ... ) > [variable] >  ( variable, ... );  //synchronous  
    ( data, ... ) > [variable] >> ( variable, ... );  //asynchronous  

* Parameters can have multiple variables, constants or null values.  
* Return values can have multiple variables.  
* End with __`;`__  

##### 11. Expressions
* Arithmetic : `+` `-` `*` `/`
* Bit : `&` `|` `!` `^`
* Comparison : `>` `>=` `<` `<=` `==` `!=`
* Logic : `&&` `||` `~`

* Expression can use external variables or define new variables. 
* Multiple expressions are separated by `;`.
* The `"1"` is logical `true` and `"0"` is logical `false`.

##### 12. Format string  
* Replace the variable name in the string with a value.
