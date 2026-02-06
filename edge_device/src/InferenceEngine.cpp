#include "InferenceEngine.h"  //works because CMakeLists has include path added
#include <iostream>

int InferenceEngine::loadModel(std::string model_path){
	model = tflite::FlatBufferModel::BuildFromFile(model_path.c_str());
	if(!model){
		std::cerr << "FAILED TO LOAD MODEL" << std::endl;
		return -1;
	}
	
	tflite::InterpreterBuilder(*model, resolver)(&interpreter);
	if (!interpreter){
		std::cerr << "FAILED TO CONSTRUCT INTERPRETER!" << std::endl;
		return -1;
	}
	
	if (interpreter->AllocateTensors() != kTfLiteOk){
		std::cerr << "FALIED TO ALLOCATE TENSORS!!" << std::endl;
		return -1;
	}
	
	return 0;
}

std::vector<float> InferenceEngine::runInference(cv::Mat &input_image){
	//resize and normalize
	cv::Mat resized;
	cv::resize(input_image,resized,cv::Size(224,224));
	
	cv::cvtColor(resized, resized,cv::COLOR_BGR2RGB);
	
	resized.convertTo(resized,CV_32FC3, 1.0f/255.0f);
	
	//copy to input tensor
	float* input_data = interpreter->typed_input_tensor<float>(0);
	if (!input_data){return {};}
	
	memcpy(input_data,resized.data,resized.total() * resized.elemSize());
	
	//Invoke
	if (interpreter->Invoke() != kTfLiteOk){
		std::cerr<<"Inference Failed" << std::endl;
		return {};
	}
	
	//getting output
	float* output_data = interpreter->typed_output_tensor<float>(0);
	return {output_data[0], output_data[1]};
	
	
	//NOTE : results[0] is alert and results[1] is drowsy
}

