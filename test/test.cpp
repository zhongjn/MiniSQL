#include "test.h"
#include"../index_manager.h"
#include"../record_manager.h"
#include"../index.h"
#include"../files.h"
vector<pair<string, void(*)()>> test_cases;

using namespace std;

/*int main() {
	int cur = 1;
	int len = test_cases.size();
	for (auto& p : test_cases) {
		auto& test = p.first;
		cout << "[" << cur << "/" << len << "] " << test << " : ";
		void (*fp)() = p.second;
		try {
			fp();
			cout << "Passed" << endl;
		}
		catch (AssertionError & ex) {
			cout << "Assertion failed! " << ex.what() << endl;
		}
		catch (exception & ex) {
			cout << "Unhandled error! " << ex.what() << endl;
		}
		cur++;
	}
	system("pause");
	return 0;

}*/
int main()
{
	BlockManager bm;
	RecordManager rm(&bm);
	Relation rel("test_table");
	Field f_id("id", Type::create_INT());
	f_id.unique = true;
	rel.fields.push_back(f_id);
	//rel.fields.push_back(Field("name", Type::create_CHAR(20)));
	//rel.fields.push_back(Field("contact", Type::create_CHAR(20)));
	rel.update();
	IndexManager x(&bm);
	for (int i = 1; i <= 10000; i++)
	{
		Value v;
		v.INT = i;
		RecordPosition k;
		k.pos = i;
		x.add_item(rel, 0, v, k);
		cout << i << " ";
		

	}
	/*for (int i = 1; i <= 1000; i++)
	{
		Value v;
		v.INT = i;
		Nullable<RecordPosition> k1 = x.find(rel, 0, v);
		cout << i << " ";
		cout << k1->pos << endl;
	}*/
	IndexUsage iu;
	iu.field_index = 0;
	Value from_v, to_v;
	from_v.INT = 1;
	to_v.INT = 10000;
	iu.from = from_v;
	iu.to = to_v;
	IndexIterator ii = x.get_index(rel, iu);
	while (ii.next())
	{
		cout << ii.current().pos << endl;
	}
	
	//v.INT = 32512;
	//RecordPosition k;
	//k.pos = 32512;
	//x.add_item(rel, 0, v, k);
	//Nullable<RecordPosition> k1 = x.find(rel, 0, v);
	//Nullable<RecordPosition> k1 = x.find(rel, 0, v);
	//Nullable<RecordPosition>
	//cout << k1->pos;
	//getchar();
	/*for (int i = 0; i < 100000; i++)
	{
		Value v;
		v.INT = i%1000;
		Nullable<RecordPosition> k1 = x.find(rel, 0, v);
		cout << i << " " << k1->pos<<endl;
	}

	Value v;
	v.INT = 253;*/
	//

	system("pause");



}