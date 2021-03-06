#include "QuickSortVisual.h"

//TODO: Allow custom z axis location
//Creates the necessasry meshes
void QuickSortVisual::makeObjects(float height){
	float startingX = -.5f;
	float positionY = -.5f;
	float margin = boxWidth/2.0f;
	//get max value

	for(std::size_t i = 0; i < array.size(); i++){
		float x = startingX + boxWidth*.5f +  margin*(i+1) + boxWidth*i;
		//Allow custom objects?

		std::shared_ptr<Mesh> box (new Mesh("cube.coor", "cube.poly"));
		box->calculateNormals();
		box->createGLBuffer(false, vao, vaoIndex);
		box->setupShader(shaderPrograms[i], camera);
		box->removeBoundingBox();
		//scale, translate new height/2 on y axis. Perhaps need to update dimensions after scale.
		box->scaleCenter(glm::vec3(boxWidth, array.at(i)*yScale, boxWidth));
		box->moveTo(glm::vec3(x, positionY + box->getSize().y*.5f, 0.0f));
		objects.push_back(box);
		vaoIndex++;
		//translate object so that top size is at x axis, scale y, translte back up
	}
}

//Creates an animation that moves the top block to indicate what is the last element not less than pivot
//and therefore needs to be swapped.
void QuickSortVisual::moveCompareIndicator(int original, int destination,  bool animated)
{
	std::cout << "Destination is " << destination << std::endl;

	glm::vec3 goalPosition = objects.at(destination)->getPosition();
	goalPosition.y +=  objects.at(destination)->getSize().y*.5f + boxWidth * .75f;

	if(!animated){
		compareIndicator.moveTo(goalPosition);
		return;
	}
	Animation move(Animation::POSITION);
	float travelTime = animationDuration* std::abs(original - destination) * .33f;
	move.setStart(&compareIndicator, compareIndicator.getPosition());
	move.setGoal(goalPosition, travelTime, Animation::ELASTIC_OUT);
	animations.push_back(move);
}

//Creates an animation that moves bottom block to indicate where our array index is pointing at.
void QuickSortVisual::moveIndexIndicator(int location, bool delay, bool animated){

	glm::vec3 goalPosition = objects.at(location)->getPosition();
	goalPosition.z +=  boxWidth;
	goalPosition.y = -.5f;

	Animation move(Animation::POSITION);

	if(!animated){
		indexIndicator.moveTo(goalPosition);
		return;
	}
	//float travelTime = animationDuration* std::abs(a - b)*.75f;
	move.setStart(&indexIndicator, indexIndicator.getPosition());
	move.setGoal(goalPosition, animationDuration, Animation::QUAD_OUT);
	if(delay){
		///adds a blank animation to serve as a wait time. Simplest thing I could do in a pinch
		Animation dummy(Animation::POSITION);
		dummy.setStart(&blank, blank.getPosition());
		dummy.setGoal(blank.getPosition(), animationDuration, Animation::NONE);
		move.chain(dummy);
	}
	animations.push_back(move);
}

//how many ms it takes to go across whole array
const float QuickSortVisual::ANIMATION_UNIT = 3500;

QuickSortVisual::QuickSortVisual(const std::vector<int>& values, Camera* camera):
	array(values), camera(camera), compareIndicator("cube.coor", "cube.poly", true),
	indexIndicator("cube.coor", "cube.poly"), blank("cube.coor", "cube.poly") {

	animationScale = 1;
	float elements = (float)values.size();
	scaleSpeed(100);
	poppedStack = false;
	havePivot = false;
	finished = false;
	vaoIndex = 0;
	left = 0;
	previousScanner = 0;
	step = -1;
	scanner = 0;
	paused = false;
	myTime = 0;
	stepMode = false;
	lastTime = 0;
	//allocate vertex array
	vao = new GLuint[values.size() + 3];
	glGenVertexArrays(values.size() + 3, vao);

	//Compute various dimensions
	height = .75f;
	yScale = height/(*std::max_element(array.begin(), array.end())); //height over the max
	boxWidth =  1.0f/array.size()/2.0f;
	
	for(auto i = array.begin(); i < array.end(); i++){
		//TODO: For efficiency, we shouldnt be initializing the same file over and over, but instead find a way to copy shader.
		GLuint shader = Angel::InitShader("vshader.vert", "fshader.frag");
		shaderPrograms.push_back(shader);

	}
	makeObjects(height);
	
	GLuint texturedShader = Angel::InitShader("textured.vert", "textured.frag");
	compareIndicator.calculateNormals();
	compareIndicator.createGLBuffer(false, vao, vaoIndex++);
	compareIndicator.loadTexture("indicator.png");
	compareIndicator.setupShader(texturedShader, camera);
	compareIndicator.removeBoundingBox();
	compareIndicator.scaleCenter(glm::vec3(boxWidth, boxWidth, boxWidth));
	
	GLuint regularShader = Angel::InitShader("vshader.vert", "fshader.frag");
	indexIndicator.calculateNormals();
	indexIndicator.createGLBuffer(false, vao, vaoIndex++);
	indexIndicator.setupShader(regularShader, camera);
	indexIndicator.removeBoundingBox();
	indexIndicator.scaleCenter(glm::vec3(boxWidth, boxWidth/4.0f, boxWidth));
	indexIndicator.setDiffuse(glm::vec4(105/255.0f, 7/255.0f, 159/255.0f, 1.0f));
	
	moveCompareIndicator(0, 0, false);
	moveIndexIndicator(0, false, false);
	//Initialize quicksort algoritmn
	right = array.size() - 1;
	stack.push(left);
	stack.push(right);
}

void QuickSortVisual::swap(int a, int b){
	int temp = array.at(a);
	array.at(a) = array.at(b);
	array.at(b) = temp;
	std::shared_ptr<Mesh> tempMesh = objects.at(a);
	objects.at(a) = objects.at(b);
	objects.at(b) = tempMesh;
}

//Marks the mesh of the pivot value to be a different color.
void QuickSortVisual::markPivot(int i){
	//These are supposed to be materials for turquoise
	objects.at(i)->setSpecular(glm::vec4(0.297254f,	0.30829f, 0.306678f, 1.0f), .1f*128.0f);
	objects.at(i)->setDiffuse(glm::vec4(0.396f, 0.74151f, 0.69102f, 1.0f));
	/*
	Animation rotate(Animation::ROTATE);
	rotate.setStart(objects.at(i), glm::vec3(0.0f, 0.0f, 0.0f));
	rotate.setGoal(glm::vec3(0.0f, 45.0f, 0.0f), animationDuration, Animation::SINE_OUT);
	animations.push_back(rotate);
	*/
}

//Makes rectangles outside of focused group semi transparent. 
//Originally wanted to copy focused group, put them forward in the z axis, 
//and when the sorted, push them back in, replacing the unsorted ones. 
void QuickSortVisual::focus(int a, int b){

	for(std::size_t i = 0; i < objects.size(); i++){
		if(i >= a && i <= b){
			//objects.at(i)->setAlpha(1.0f);
			Animation changeAlpha(Animation::TRANSPARENCY);
			changeAlpha.setStart(objects.at(i).get(), glm::vec3(objects.at(i)->getAlpha()));
			changeAlpha.setGoal(glm::vec3(1.0f), animationDuration*.75f, Animation::LINEAR);
			animations.push_back(changeAlpha);
		}
		else{
			Animation changeAlpha(Animation::TRANSPARENCY);
			changeAlpha.setStart(objects.at(i).get(), glm::vec3(objects.at(i)->getAlpha()));
			changeAlpha.setGoal(glm::vec3(.4f), animationDuration*.75f, Animation::LINEAR);
			animations.push_back(changeAlpha);
			//objects.at(i)->setAlpha(.5f);
		}
	}
}

/*
	1. Bring out both. A into positive Z axis, B into negative Z axis.
	2. Take current locations of both, swap positions excluding z axis. Duration is same for both.
	3. Push back into line.
*/
void QuickSortVisual::swapAnimation(int a, int b){

	Mesh* meshA = objects.at(a).get();
	Mesh* meshB = objects.at(b).get();
	float displacementA = meshA->getSize().y*.5f;
	float displacementB = meshB->getSize().y*.5f;

	glm::vec3 aPosition = objects.at(a)->getPosition();
	glm::vec3 bPosition = objects.at(b)->getPosition();
	float z = aPosition.z;
	//Movement time depends on how far things are.
	float travelTime = animationDuration* std::abs(a - b);
	Animation offsetA(Animation::POSITION);
	offsetA.setStart(meshA, aPosition);
	offsetA.setGoal(glm::vec3(aPosition.x, aPosition.y, z + boxWidth), animationDuration);
	
	Animation switchA(Animation::POSITION);
	switchA.setStart(meshA, glm::vec3(aPosition.x, aPosition.y, z + boxWidth));
	switchA.setGoal(glm::vec3(bPosition.x, aPosition.y, z + boxWidth), travelTime, Animation::QUAD_OUT);

	Animation returnA(Animation::POSITION);
	returnA.setStart(meshA, glm::vec3(bPosition.x, aPosition.y, z + boxWidth));
	returnA.setGoal(glm::vec3(bPosition.x, aPosition.y, z), animationDuration);
	
	Animation offsetB(Animation::POSITION);
	offsetB.setStart(meshB, bPosition);
	offsetB.setGoal(glm::vec3(bPosition.x, bPosition.y, z - boxWidth), animationDuration);

	Animation switchB(Animation::POSITION);
	switchB.setStart(meshB, glm::vec3(bPosition.x, bPosition.y, z - boxWidth));
	switchB.setGoal(glm::vec3(aPosition.x, bPosition.y, z - boxWidth), travelTime, Animation::QUAD_OUT);

	Animation returnB(Animation::POSITION);
	returnB.setStart(meshB, glm::vec3(aPosition.x, bPosition.y, z - boxWidth));
	returnB.setGoal(glm::vec3(aPosition.x, bPosition.y, z), animationDuration);

	//chain sort of in reverse because of copy by value. consider changin
	switchA.chain(returnA);
	offsetA.chain(switchA);

	switchB.chain(returnB);
	offsetB.chain(switchB);
	
	animations.push_back(offsetA);
	animations.push_back(offsetB);
}

//Creates an animation for:
/*
for i = left to right - 1
				if A.at(i) &lt;= pivotValue
					swap A.at(storeIndex) with A.at(i)
					storeIndex++ //only increment if swapped
*/
//the i in the loop is equal to step in my code
//Note this will only be called when animation queue is empty;
int QuickSortVisual::partitionAnimationStep(int left, int right, int step, int& scanner, int pivotIndex){
	
	if(step == 0)
		markPivot(pivotIndex);
	if(previousScanner != scanner)
		moveCompareIndicator(previousScanner, scanner);
	previousScanner = scanner;
	/* No idea what this is for anymore.
	if(step == -1){
		markPivot(pivotIndex);
		swap(pivotIndex, right);
		swapAnimation(pivotIndex, right);
		return scanner; 
	}
	*/
	
	int pivotValue = array.at(right);
	//instead of having for loop, iterate by steps
	int i = left + step;
	if(i < right){
		std::cout << "Check if scanner <= pivotValue. " << std::endl;
		if(array.at(i) <= pivotValue){
			//delay so that swapping can go after
			moveIndexIndicator(i, true);
			//chain this move index before swapping
			swapAnimation(i, scanner);
			std::cout << "Swapping array[" << i << "] with array[" << scanner <<"]" << std::endl;
			swap(i, scanner);
			scanner++;
		}
		else //no need for a delay.
			moveIndexIndicator(i, false);
	}

	else {
		moveIndexIndicator(i, true);
		swapAnimation(right, scanner);
		swap(right, scanner);
	}

	return scanner;
}

//A non recursive version of this shizzle
void QuickSortVisual::quickSortStep(int& left, int& right, int& step, int& scanner){
	//This is equivalent to while !stack.empty(), but it would be bad to check while partition
	//animation is in progress.
	if(!finished){
		if(!poppedStack){
			right = stack.top();
			stack.pop();
			left = stack.top();
			stack.pop();
			scanner = left; //Start partition function's scanner at 0
			focus(left, right);
			poppedStack = true;
		}
		if(step == -1){
			//get a random pivot
			//pivotIndex = left + (std::rand() % (right - left + 1));

			//For simplicity, always pick rightmost as pivot
			pivotIndex = right;
			//if we put back random, get rid of this
			step++;
		}
		//This happens when partitionAnimation has finished.
		if(step > right - left){ //the extra step is for first pivot switch
			step = -1;
			std::cout << "Pivot is " << pivot << std::endl;
			poppedStack = false;
			std::cout << "Ready to Recurse" << std::endl;   
			havePivot = true;
			markPivot(pivot);
		}

		else
			pivot = partitionAnimationStep(left, right, step++, scanner, pivotIndex);
		//the value of this pivot value is ignored until here.
		if(havePivot){
			//Right side of pivot is added to stack if there are elements to right of it.
			if(pivot + 1 < right)
			{
				stack.push(pivot + 1);
				stack.push(right);
			}
			
			//Left side of pivot is added to stack in order to be partitioned.
			//It is equivalent to calling quicksort recursively.
			if(pivot -  1> left)
			{
				stack.push(left);
				stack.push(pivot - 1);
			}
			
			havePivot =  false;
			
			if(stack.empty())
				finished = true;
		}
	}
	else 
	{
		std::cout << "Done." << std::endl;
	}
}


/*
How to cycle animations:
	Call update on each animation, passing in time
	When an animation finishes playing and it doesnt have a link to another animation, destroy it
	If it does have linked animations, store in temporary list and then join the animation list with the temp list.

*/
void QuickSortVisual::updateAnimations(int time){
	auto i = animations.begin();
	
	std::list<Animation> links;
	while(i != animations.end()){
		// std::cout << "still animating" << std::endl;
		bool hasEnded = (*i).hasEnded();
		if(hasEnded){
			if((*i).containsLink())
				links.push_back(*(*i).getLink()); //need to make sure it doesnt activate.
			i = animations.erase(i); //set iterator to the element that follows the one we erased.
		}
		else {
			(*i).update(time);
			++i;
		}
	}
	//merge list of animation links
	animations.splice(animations.begin(), links);
}

/*
	Step Mode logic.
	1. When user clicks step, state should be paused.
	2. Finish any current animations and pause again.
*/
void QuickSortVisual::update(int realTime){
	if(!paused){
		// Allows us to continue animation were we left of.
		myTime += realTime - lastTime;  
		updateAnimations(myTime);
		if(animations.size() == 0 && !finished){
			//if stepMode is still true and animation is finished
			if(stepMode ) { 
				paused = true;
				stepMode = false;
			}
			quickSortStep(left, right, step, scanner);
		}

	}
	else {
		if(stepMode){
			// finish current animations 
			if(animations.size() > 0)
				paused = false;
		}
	}
	lastTime = realTime;
}
//Draw the actual meshes.
void QuickSortVisual::draw(){
	compareIndicator.draw();
	indexIndicator.draw();
	for(auto i = objects.begin(); i < objects.end(); i++){
		(*i)->draw();
	}
}

QuickSortVisual::~QuickSortVisual()
{
	delete[] vao;
	vao = NULL;
}