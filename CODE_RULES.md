# Coding rules

## References

> _Nothing here is dogma. There is no single way._<br>
> _But when in doubt, following these rules might keep you out of trouble for a little longer._

> I have yet to see any problem, however complicated, which when looked at in the right way, did not become still more complicated.
> Anderson's Law

- https://www.youtube.com/watch?v=rX0ItVEVjHc
- https://www.youtube.com/watch?v=QM1iUe6IofM
- https://www.youtube.com/watch?v=qWJpI2adCcs
- https://www.youtube.com/watch?v=FyCYva9DhsI

## Zen

- Explicit is better than implicit.
- Solving problems you don't have creates problems you now have.
- The absence of structure is more structured than bad structure.
- The only thing a program does is transforming data: data in -> transformed data out.
	- Write the simplest program possible to do that.
	- Store the minimum amount of state required to do that.
		- Caching errors (keeping derived states properly synchronized) are the most common source of bugs in programs.
		- Storing useless states, doing more than strictly necessary is a one way street: https://youtu.be/pgoetgxecw8?t=546
- Writing concise, efficient solution to a problem requires as much context as possible.
- Debugging code takes 10x more brain power than writing it.
	- Don't write smart code, don't use arcane constructs or exotic syntax shortcuts.

## Pass the smallest possible data type as parameters to functions

- Function dependencies are absolutely clear.
- A complex function might take a large number of parameters, this is not a problem but a feature.
	- Calling a complex function requires understanding it, its effects and potential side effects.
- Do not pack parameters to a function in a structure.
	- Parameters from heterogeneous sources need additional preparation before they can be used by that function.
	- That function still takes as many parameters plus one extra for the packing structure.
	- The temptation to pack more _"related"_ things in that structure will creep up and obscure real dependencies of the function.
		- Will most likely lead to storing unnecessary states.
	* Caveat: This might be justified for functions which are part of an API requiring ABI backward compatibility.

## Do not aim for code reusability

- Encourages solving problems you don't have.
- Code reusability is a good thing for very generic problems that are well understood and have been properly solved (eg. containers), most problems do not fall into this category.

## const everything by default

- A const variable is one less thing to think about while reading through the code.

## Declare functions static by default

- Non-static functions might potentially be used elsewhere in the program even if you did not intend them to be.
- When modifying a non static function, care needs to be taken with potential side effects in the rest of the codebase.

## Avoid deep callbacks

- Callbacks are good old spaghetti code squared.
- It is most likely that callbacks will be written without sufficient understanding of the context they are being called from.
- When placing a callback hook in your program you have the responsibilty of designing the context it can access:
	- This encourages solving problems you don't have. _(What will a callback need to efficiently solve hypothetic problem A or B? What if problem C arises?)_
	- Almost guaranteed sub-optimal solution.
		- Can't reason in terms of many if the call site was not designed for it.
- Split code into pre/post functions so that the calling context is fully under control of the programmer implementing future functionalities.

## Prefer packing data into elementary streams not objects (AOS vs SOA)

- Hardware is designed to efficiently process streams of homogeneous data.
- It is trivial and very fast to reorganize data streams into a layout more suited to efficiently solve a particular problem.
	- It is trivial but **very** slow to do the same thing with objects. _(Cache misses lead to death by a thousand cuts, no amount of profiling will save you.)_

## Avoid classes

- They couple code and data.
	- Data belonging to a class might need to be transformed in a new way leading to:
		- Bloat (multiple responsabilities) by exposing additional processing methods.
		- Exposing direct access to the data it manages (adieu encapsulation).
		- Restructuration of the object model to accomodate new requirements (potentially very time consuming with a large impact on existing code).
	- A method of your class might be useful to transform external data leading to:
		- Static methods to process external data (use functions!).
- They require data to be fit into an object model, solving philosophical questions along the way (is a cat an animal or does it belong to the animal kingdom? does it even quack?).
- Thinking in term of objects often requires decoupling from the global context which leads to inefficient solutions. _("Where there's one, there's many, look on the time axis" Mike Acton)_
	- Inefficient data storage.
	- Limited batch processing and/or parallelization.
- If the transformations your data needs to undergo are varied, the object model will have to be split into arbitrary smaller pieces until it can be processed in all the ways required.
	- This encourages trying to design an *elegant* or *perfect* model which is 100% a waste of time.
		- The object model does not solve problems: code and data transformations do.
- Requirement changes and evolutions will break the current model design.
	- Cannot anticipate the future.
		- Trying to do so will lead to solving problems you don't have.
- You now need to master both your data and its object model.
- Classes are ressources that need their lifetime managed.
	- Directly thinking about data makes a whole class of problems immediately vanish. _(eg. `shared_ptr<MyThing>`)_

## Be very cautious around OOP

- OOP sells the illusion that you can forget about the data and interact with it through abstract objects and concepts. As the true nature of the program is lost this leads to:
	- Terrible software architecture as data undergoes more and more unnecessary transformations to conform to an abstract object model.
	- Terrible performance due to repetitive unnecessary transformations and poor general data layout.
	- Maintanability nightmare as even straightforward transfomations involve tens of different objects and method calls.
	- Encourages stagnation as programmers become little more than Lego problem solvers.
- As a programmer, the number one priority you have is understanding your data and the transformations it undergoes.
	- Then implement those transformations storing as few states as possible.

## Subjective: Do not split large functions into smaller ones

- If that function represents a monolithic task you only make that fact unclear by splitting it.
- Moving code to separate functions for readability purposes does not change a thing:
	- That code is still executed and part of the function.
	- You now need to track it down to understand the function.
- Use scope to isolate conceptual segments of code inside a long function.
- Any modern IDE supports code folding to collapse a scope.
