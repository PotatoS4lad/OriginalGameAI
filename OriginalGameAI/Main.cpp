
# include <Siv3D.hpp>
# include "Qlearning_AI.hpp"


struct Sequence :IEffect
{
	const Font font;
	const String first;

	Sequence()
		:font(15)
	{

	}

	bool update(double t) override
	{
		font(L"先攻は", first, L"です").drawAt(240, 300);
	}
};

class OriginalGame :IndexData
{
private:
	struct Card
	{
		Card(RoundRect _rect, int32 _number)
			:card(_rect), number(_number), side(true), used(false) {}

		RoundRect card;
		int32	  number;
		bool	  side;	//trueならば表,falseならば裏.
		bool	  used;	//trueならば使用済み.
	};
	Array<Card> cards_p1;
	Array<Card> cards_p2;

	const RoundRect front_p1;
	const RoundRect back_p1;
	const RoundRect front_p2;
	const RoundRect back_p2;
	size_t	index_frontp1;
	size_t	index_backp1;
	size_t	index_frontp2;
	size_t	index_backp2;

	SelectableCard used_p1;
	SelectableCard used_p2;
	//size_t	used_p1[4] = { 9 };
	//size_t	used_p2[4] = { 9 };

	const Font font;

	double	winning_p;
	double	wincount_m = 0, wincount_o = 0;
	int32	battlecount = 0;

	Array<Vec2> wins;

	bool	learning = false;
	int32	l_count = 0;
	int32	l_limit = 0;

	bool	attackside;	//先攻か後攻かを示す,trueで手前が先攻,falseで奥が先攻.

	enum class Judg
	{
		mWin = 0,
		Draw,
		oWin
	}judg;

	enum class Phase
	{
		Ready = 0,
		First,
		After,
		Judgment,
		Replay
	}phase;

	/**************************/
	// 各関数内で使用する変数 //
	/**************************/
	bool	flag_drag = false;
	bool	dragedfront = false;
	bool	dragedback = false;
	size_t	number_drag = 0;
	Vec2	posmemory;

	QlearningAI ai;

public:
	OriginalGame()
		:font(15),
		front_p1(RoundRect(RectF(54, 72), 5).setCenter(200, 350).stretched(1)),
		back_p1(RoundRect(RectF(54, 72), 5).setCenter(280, 350).stretched(1)),
		front_p2(RoundRect(RectF(54, 72), 5).setCenter(280, 250).stretched(1)),
		back_p2(RoundRect(RectF(54, 72), 5).setCenter(200, 250).stretched(1)),
		used_p1(0, 0, 0, 0, 0, 0, 0, 0),
		used_p2(0, 0, 0, 0, 0, 0, 0, 0)
	{
		for (auto i : step(8))
		{
			cards_p1.emplace_back(RoundRect(17 + i * 56, 400, 54, 72, 5), i);
			cards_p2.emplace_back(RoundRect(17 + i * 56, 130, 54, 72, 5), i);
		}
	}

	~OriginalGame() {}

	/*   p2シャッフル   */
	/*  p2のカードをランダムに並べ替えする.  */
	void shuffle()
	{
		Array<Card> tmp;
		for (auto const card : cards_p2)
		{
			tmp.emplace_back(card);
		}
		cards_p2.clear();
		Array<int> ns;
		for (auto i : step(8))
		{
			ns.emplace_back(i);
		}
		for (auto i : step(8))
		{
			const auto a = RandomSelect(ns);
			cards_p2.emplace_back(tmp[a].card, i);
			Erase_if(ns, [=](const int& _n) {return a == _n;});
		}
	}

	/*   準備   */
	void ready()
	{
		attackside = RandomBool();
	}

	/*   順番決め   */
	/*void sequencing()	
	{
		
	}*/

	/*   カード選択   */
	void selectcard()	
	{
		if (!flag_drag)
		{
			for (auto i : step(cards_p1.size()))
			{
				if (cards_p1[i].card.leftClicked)
				{
					flag_drag = true;
					number_drag = i;
					posmemory = cards_p1[i].card.center;
					break;
				}
			}
		}
		else
		{
			cards_p1[number_drag].card.setCenter(Mouse::PosF());

			if (Input::MouseL.released)
			{
				if (front_p1.mouseOver && !dragedfront)
				{
					cards_p1[number_drag].card.setCenter(front_p1.center);
					dragedfront = true;
					index_frontp1 = number_drag;
				}
				else if (back_p1.mouseOver && !dragedback)
				{
					cards_p1[number_drag].card.setCenter(back_p1.center);
					dragedback = true;
					index_backp1 = number_drag;
				}
				else
				{
					cards_p1[number_drag].card.setCenter(posmemory);
				}

				flag_drag = false;
			}
		}
	}
	
	/*   AI選択   */
	void qaiselect(uint32 _pm)
	{
		bool	b, c;
		size_t	m, o;
		for (size_t i = 0;i < s_cards.size();i++)
		{
			b = c = true;
			for (size_t j = 0; j < 8; j++)
			{
				if (s_cards[i].sc[j] != used_p1.sc[j])
				{
					b = false;
				}
				if (s_cards[i].sc[j] != used_p2.sc[j])
				{
					c = false;
				}
				if (b)
				{
					m = i;
				}
				if (c)
				{
					o = i;
				}
			}
		}

		size_t f;
		if (_pm == 0)
		{
			f = attackside ? index_frontp1 : 8;
		}
		else if (_pm == 1)
		{
			f = attackside ? 8 : index_frontp2;
		}
		ai.select(attackside, m, o, f);
	}

	/*   ランダム選択   */
	void randselect()
	{
		Array<size_t> ns;
		for (auto i : step(8))
		{
			if (!cards_p2[i].used)
			{
				ns.emplace_back(i);
			}
		}
		auto s1 = 0, s2 = 0;
		do
		{
			s1 = RandomSelect(ns);
			s2 = RandomSelect(ns);
		} while (s1 == s2);
		cards_p2[s1].card.setCenter(front_p2.center);
		cards_p2[s2].card.setCenter(back_p2.center);
	}

	/*   勝敗判定   */
	void judgment()
	{
		const auto p1total = index_frontp1 + index_backp1;
		const auto p2total = index_frontp2 + index_backp2;
	}

	/*   メインとなる更新   */
	void update(uint32 _pm, bool _lm, bool _start, uint32 _t, bool _learn, bool _stop)
	{
		selectcard();

		/*if (Input::MouseL.clicked)
		{
			//cards_p1[4].card.moveBy(0, 90);
			//cards_p2[3].card.moveBy(0, -90);
		}*/

		switch (phase)
		{
		case OriginalGame::Phase::Ready:
			ready();
			phase = Phase::First;
			break;
		case OriginalGame::Phase::First:
			if (attackside)
			{
				selectcard();
			}
			else
			{
				qaiselect(_pm);
			}
			break;
		case OriginalGame::Phase::After:
			if (!attackside)
			{
				selectcard();
			}
			else
			{
				qaiselect(_pm);
			}
			break;
		case OriginalGame::Phase::Judgment:
			judgment();
			if (_pm == 0)
			{

			}
			else if (_pm == 1)
			{

			}
			phase = Phase::Replay;
			break;
		case OriginalGame::Phase::Replay:
			break;
		default:
			break;
		}

	}

	/*   描画   */
	void draw() const
	{
		Line(10, 120, 470, 120).draw(ColorF(0, 0.8));
		Line(10, 480, 470, 480).draw(ColorF(0, 0.8));

		font(L"Used").drawAt(240, 20);
		font(L"Used").drawAt(240, 580);

		front_p1.draw(ColorF(0, 0.2)).drawFrame(0, 2, ColorF(0, 0.8));
		font(L"表").drawAt(front_p1.center);

		back_p1.draw(ColorF(0, 0.2)).drawFrame(0, 2, ColorF(0, 0.8));
		font(L"裏").drawAt(back_p1.center);

		front_p2.draw(ColorF(0, 0.2)).drawFrame(0, 2, ColorF(0, 0.8));
		font(L"表").drawAt(front_p2.center);

		back_p2.draw(ColorF(0, 0.2)).drawFrame(0, 2, ColorF(0, 0.8));
		font(L"裏").drawAt(back_p2.center);

		for (const auto card : cards_p1)
		{
			card.card.draw().drawFrame(1, 0, Palette::Black);
			if (card.card.center == back_p1.center)
			{
				RoundRect(RectF(42, 56), 4).setCenter(back_p1.center).draw(Color(220, 90, 90)).drawFrame(0, 1, Palette::Black);
			}
			font(card.number == 0 ? L"J" : Format(card.number)).drawAt(card.card.center, Palette::Black);
		}
		auto j = 0;
		for (const auto card : cards_p2)
		{
			PutText(j).at(card.card.center.movedBy(0, -50));
			++j;
			card.card.draw().drawFrame(1, 0, Palette::Black);
			RoundRect(RectF(42, 56), 4).setCenter(card.card.center).draw(Color(220, 90, 90)).drawFrame(0, 1, Palette::Black);
		}

		//
		Rect(0, 600, Window::Width(), 120).draw(Color(255, 255, 140)).drawFrame(0, 1, Palette::Black);
		for (auto i : step(6))
		{
			Line(38, 610 + i * 20, Window::Width(), 610 + i * 20).draw(ColorF(0.5, 0.7, 1.0, 0.6));
		}
		Line(40, 610, 40, 710).draw(ColorF(0.5, 0.7, 1.0, 0.6));
	}
};

class Menu
{
public:
	Menu()
		:gui(GUIStyle::Default)
	{
		gui.addln(L"playmode", GUIRadioButton::Create({ L"Player VS AI",L"AI VS AI" }));
		gui.addln(L"learnmode", GUIToggleSwitch::Create(L"OFF", L"ON", false));
		gui.addln(L"start", GUIButton::Create(L"  Start!  "));
		gui.button(L"start").enabled = false;

		gui.add(GUIHorizontalLine::Create(1));
		gui.addln(L"times", GUIRadioButton::Create({ L"100",L"500",L"1000",L"5000",L"10000" }));
		gui.addln(L"learning", GUIButton::Create(L"Learning!"));
		gui.addln(L"stop", GUIButton::Create(L"  Stop...  "));
		gui.button(L"learning").enabled = gui.button(L"stop").enabled = false;

		gui.setTitle(L"Menu");
		gui.setPos(480, 0);
		gui.style.borderRadius = 0;
		gui.style.movable = false;
		gui.style.shadowColor = ColorF(0, 0);
		gui.style.padding = Padding(14, 14, 200);
	}
	~Menu(){}

	void update()
	{
		if (getpmode != 100)
		{
			gui.button(L"start").enabled = true;
		}
		if (gettimes != 100)
		{
			gui.button(L"learning").enabled = gui.button(L"stop").enabled = true;
		}
	}

	Property_Get(uint32, getpmode) const { return gui.radioButton(L"playmode").checkedItem.value_or(100); }
	Property_Get(bool, getlmode) const { return gui.toggleSwitch(L"learnmode").isRight; }
	Property_Get(bool, getstart) const { return gui.button(L"start").pushed; }
	Property_Get(uint32, gettimes) const { return gui.radioButton(L"times").checkedItem.value_or(100); }
	Property_Get(bool, getlearn) const { return gui.button(L"learning").pushed; }
	Property_Get(bool, getstop) const { return gui.button(L"stop").pushed; }

private:
	GUI gui;
};
void Main()
{
	Window::Resize(640, 720);
	Graphics::SetBackground(Color(160, 90, 30));

	OriginalGame originalgame;

	Menu menu;

	while (System::Update())
	{
		menu.update();

		const auto pm	 = menu.getpmode;
		const auto lm	 = menu.getlmode;
		const auto start = menu.getstart;
		const auto t	 = menu.gettimes;
		const auto learn = menu.getlearn;
		const auto stop  = menu.getstop;
		originalgame.update(pm, lm, start, t, learn, stop);

		originalgame.draw();

		PutText(Mouse::Pos()).from(Mouse::Pos().movedBy(10, 10));
	}
}
