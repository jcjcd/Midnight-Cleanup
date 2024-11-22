#pragma once
class IndexCreator
{
public:
	IndexCreator() = default;

	IndexCreator(uint32_t num)
	{
		m_Table = new uint32_t[num];
		m_MaxIndex = num;
		m_AllocatedCount = 0;
		memset(m_Table, 0, sizeof(uint32_t) * num);

		for (uint32_t i = 0; i < num; i++)
		{
			m_Table[i] = i;
		}
	}

	~IndexCreator()
	{
		if (m_Table)
		{
			delete[] m_Table;
			m_Table = nullptr;
		}
	}

	uint32_t Allocate()
	{
		uint32_t index = -1;
		if (m_AllocatedCount < m_MaxIndex)
		{
			index = m_Table[m_AllocatedCount];
			m_AllocatedCount++;
			return index;
		}
		return index;
	}

	void Free(uint32_t index)
	{
		if (m_AllocatedCount <= 0)
		{
			__debugbreak();
		}
		m_AllocatedCount--;
		m_Table[m_AllocatedCount] = index;
	}

	void Clear()
	{
		m_AllocatedCount = 0;
		memset(m_Table, 0, sizeof(uint32_t) * m_MaxIndex);
		for (uint32_t i = 0; i < m_MaxIndex; i++)
		{
			m_Table[i] = i;
		}
	}


	void Resize(uint32_t num)
	{
		if (m_Table)
		{
			delete[] m_Table;
			m_Table = nullptr;
		}

		m_Table = new uint32_t[num];
		m_MaxIndex = num;
		m_AllocatedCount = 0;
		memset(m_Table, 0, sizeof(uint32_t) * num);

		for (uint32_t i = 0; i < num; i++)
		{
			m_Table[i] = i;
		}
	}

private:
	uint32_t m_MaxIndex = 0; 
	uint32_t* m_Table = nullptr;
	uint32_t m_AllocatedCount = 0;
};

