#ifndef _SINGLETON_H
#define _SINGLETON_H
#pragma once

template<typename T>
class SingletionEX
{
	friend T;
private:
	SingletionEX(){}
	virtual ~SingletionEX(){}



	//利用此类析构函数释放单例实例内存  
	class CCarbageCollector
	{
	public:
		~CCarbageCollector()
		{
			if (SingletionEX<T>::pObject != nullptr)
			{
				delete SingletionEX<T>::pObject;
			}
		}
	};

protected:
	//提供给单例类的初始化接口      
	virtual void Quit() = 0;

public:
	static T* volatile pObject;
	//获取单例  
	inline static T* getInstance()
	{
		//用于系统退出时释放单例实例的内存  
		static SingletionEX<T>::CCarbageCollector CarCo;
		if (nullptr == pObject)
			pObject = new (T)();
		return pObject;
	}

};

template<typename T>
T* volatile SingletionEX<T>::pObject = nullptr;

#define SINGLETON_INIT_EX(ClassType) \
    friend ClassType* SingletionEX<ClassType>::getInstance();\
    friend SingletionEX<ClassType>::CCarbageCollector;\
private:\
    virtual ~ClassType(){ Quit(); }\
public:\
    virtual void Quit()


#endif//_SINGLETON_H

