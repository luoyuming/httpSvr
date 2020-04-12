/*
*  12797202@qq.com   name: luoyuming mobile 13925236752
*
* Copyright (c) 2020 www.dswd.net
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
* LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
* OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "taskQueue.h"


CTaskQueue::CTaskQueue() :m_bQuit(false)
{
	
}

void CTaskQueue::setQuit()
{	
	m_bQuit = true;	
	m_cvCmd.notify_all();
}

void CTaskQueue::inputCmd(std::shared_ptr<PACKAGE_INFO> & cmd)
{
	std::unique_lock <std::mutex> lck(m_lockCmd);
	m_CmdQ.push_back(cmd);
	m_cvCmd.notify_all();
}

bool CTaskQueue::full()
{
	bool bResult = false;
	int len = getLen();
	if (len >= MAX_QUEUE_LEN) {
		bResult = true;
	}
	return bResult;
}

int CTaskQueue::getLen()
{
	int size = 0;
	{ 
		std::lock_guard <std::mutex> lck(m_lockCmd);
		size = static_cast<int>(m_CmdQ.size());
	}
	return size;
}

bool CTaskQueue::getCmd(std::shared_ptr<PACKAGE_INFO> & cmd)
{
	std::unique_lock <std::mutex> lck(m_lockCmd);
	while (m_CmdQ.empty() && (!m_bQuit))
	{
		m_cvCmd.wait(lck);
	}
	if (m_bQuit) {
		return false;
	}
	cmd = std::move(m_CmdQ.front());
	m_CmdQ.pop_front();
	return true;
}