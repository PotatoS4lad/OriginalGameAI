#pragma once

struct Decision
{
	Decision(int32 _d1, int32 _d2, bool _s)
		:d1(_d1), d2(_d2), side(_s) {}
	int32	d1, d2;
	bool	side;	//trueならd1が表,falseならd2が表であることを示す.
};

struct SelectableCard
{
	SelectableCard(int32 _s0, int32 _s1, int32 _s2, int32 _s3, int32 _s4, int32 _s5, int32 _s6, int32 _s7)
		:sc{ _s0, _s1, _s2, _s3, _s4, _s5, _s6, _s7 }{}
	int32	sc[8];	// 0で選択可,1で不可. 普通と逆なので注意.  s0はジョーカーを示す
};

struct IndexData
{
	Array<Decision> decisions;
	Array<SelectableCard> s_cards;

	IndexData()
	{
		//decisionsの中身
		for (size_t i = 0;i < 8;i++)
		{
			for (size_t j = i + 1; j < 8; j++)
			{
				for (size_t k = 0; k < 2; k++)
				{
					decisions.emplace_back(i, j, k ? true : false);
				}
			}
		}

		//s_cardsの中身
		for (size_t i = 0; i < 256; i++)
		{
			s_cards.emplace_back(i % 2, (i / 2) % 2, (i / 4) % 2, (i / 8) % 2, (i / 16) % 2, (i / 32) % 2, (i / 64) % 2, (i / 128) % 2);
		}
		
		const auto len = s_cards.size();
		for (size_t i = 0;i < len;i++)
		{
			for (auto s_card = s_cards.begin();s_card != s_cards.end();s_card++)
			{
				const auto stotal = s_card->sc[0] + s_card->sc[1] + s_card->sc[2] + s_card->sc[3] + s_card->sc[4] + s_card->sc[5] + s_card->sc[6] + s_card->sc[7];
				if (4 < stotal || stotal % 2 != 0)
				{
					s_cards.erase(s_card);
					break;
				}
			}
		}
	}
};

class QlearningAI :IndexData
{
private:
	const double ALPHA;
	const double GAMMA;
	const double EPSILON;

	/*Grid<double> qvalue_myc{ 56,256,0 };
	Grid<double> qvalue_opc{ 56,256,0 };
	Grid<double> qvalue_opf{ 56,9,0 };
	Grid<double> qvalue_opb{ 56,9,0 };*/

	const size_t width = 56;
	double qv_attack [56][2];
	double qv_mycard [56][99];
	double qv_opcard [56][99];
	double qv_opfront[56][9];

	size_t memo;
	//size_t memoindex = 0;

	size_t m_s;
	size_t m_m;
	size_t m_o;
	size_t m_f;
	
public:
	QlearningAI()
		:ALPHA(0.1),
		 GAMMA(0.9),
		 EPSILON(0.1)
	{
		for (auto p : step(Point(width, 2)))
		{
			qv_attack[p.x][p.y] = Random(-0.2, 0.2);
		}
		for (auto p : step(Point(width, 99)))
		{
			qv_mycard[p.x][p.y] = Random(-0.2, 0.2);
			qv_opcard[p.x][p.y] = Random(-0.2, 0.2);
		}
		for (auto p : step(Point(width, 9)))
		{
			qv_opfront[p.x][p.y] = Random(-0.2, 0.2);
		}
	}

	~QlearningAI(){}
	

	/// <param name="_s">
	/// 先攻か後攻か
	/// </param>
	/// <param name="_m">
	/// 自分が出せるカード
	/// </param>
	/// <param name="_o">
	/// 相手が出せるカード
	/// </param>
	/// <param name="_f">
	/// 相手が表向きで出したカード
	/// </param>
	Decision select(bool _s, size_t _m, size_t _o, size_t _f)
	{
		memo = 0;
		m_s = _s, m_m = _m, m_o = _o, m_f = _f;
		if (Random() < EPSILON)
		{
			memo = Random(width - 1);
		}
		else
		{
			for (size_t i = 0; i < width; i++)
			{
				const auto n = qv_attack[i][_s] + qv_mycard[i][_m]
					+ qv_opcard[i][_o] + qv_opfront[i][_f];
				const auto m = qv_attack[memo][_s] + qv_mycard[memo][_m]
					+ qv_opcard[memo][_o] + qv_opfront[memo][_f];
				if (n > m)
				{
					memo = i;
				}
			}
		}
		return decisions[memo];
	}

	//Q値更新
	void updateq(bool _s, size_t _m, size_t _o, size_t _f)
	{
		size_t m_max = 0;
		for (size_t i = 0; i < width; i++)
		{
			const auto n = qv_attack[i][_s] + qv_mycard[i][_m]
				+ qv_opcard[i][_o] + qv_opfront[i][_f];
			const auto mm = qv_attack[m_max][_s] + qv_mycard[m_max][_m]
				+ qv_opcard[m_max][_o] + qv_opfront[m_max][_f];
			if (n > mm)
			{
				m_max = i;
			}
		}

		qv_attack[memo][m_s] = qv_attack[memo][m_s] + ALPHA * (GAMMA * qv_attack[m_max][_s] - qv_attack[memo][m_s]);
		qv_mycard[memo][m_m] = qv_mycard[memo][m_s] + ALPHA * (GAMMA * qv_mycard[m_max][_s] - qv_mycard[memo][m_s]);
		qv_opcard[memo][m_o] = qv_opcard[memo][m_s] + ALPHA * (GAMMA * qv_opcard[m_max][_s] - qv_opcard[memo][m_s]);
		qv_opfront[memo][m_f] = qv_opfront[memo][m_s] + ALPHA * (GAMMA * qv_opfront[m_max][_s] - qv_opfront[memo][m_s]);
	}

	//Q値更新
	void updateq(double _result)
	{
		qv_attack[memo][m_s] = qv_attack[memo][m_s] + ALPHA * (_result - qv_attack[memo][m_s]);
		qv_mycard[memo][m_m] = qv_mycard[memo][m_s] + ALPHA * (_result - qv_mycard[memo][m_s]);
		qv_opcard[memo][m_o] = qv_opcard[memo][m_s] + ALPHA * (_result - qv_opcard[memo][m_s]);
		qv_opfront[memo][m_f] = qv_opfront[memo][m_s] + ALPHA * (_result - qv_opfront[memo][m_s]);
	}
};