#include "index_manager.h"
#include"common.h"
IndexManager::IndexManager(BlockManager* block_mgr) :block_mgr(block_mgr){}
void IndexManager::add_index(const Relation& rel, int field_index, unique_ptr<Scanner> scanner)
{
	while (scanner->next())
	{
		Record& rec = scanner->current();
		Value temp_value = rec.values[field_index];
		RecordPosition temp_record = rec.physical_position;
		IndexManager::add_item(rel, field_index, temp_value, temp_record);
	}
}
tree_information IndexManager::get_information(const Relation& rel, int field_index,string& scheme)
{
	if (block_mgr->file_blocks(scheme) == 0)
	{
		block_mgr->file_append_block(scheme);
		BlockGuard bg_init(block_mgr, scheme, 0);
		tree_information* head = bg_init.addr<tree_information>();
		// degree = {Is_root + Is_leaf + fa + size} + data[degree] + block_pos[degree] + rec_address[degree]
		int degree = (BLOCK_SIZE - (sizeof(bool) * 2 + 2 * sizeof(int))) / (rel.fields[field_index].type.length() + sizeof(int) + sizeof(RecordPosition));
		head->degree = degree;
		head->root = -1;
		bg_init.set_modified();
	}
	BlockGuard bg_init(block_mgr, scheme, 0);
	tree_information tree_info = *(bg_init.addr<tree_information>());
	return tree_info;
}
int IndexManager::get_new_block(string& scheme)
{
	block_mgr->file_append_block(scheme);
	int new_block = block_mgr->file_blocks(scheme) - 1;
	return new_block;
}
void IndexManager::insert_value(string& scheme, Type type, const Value& value, int degree, int num, int pos)
{
	int offset = sizeof(treenode) + pos * type.length();
	BlockGuard bg(block_mgr, scheme, num);
	value.write(bg.addr<void>(offset),type);
}
void IndexManager::insert_record(string& scheme, Type type,RecordPosition record_pos, int degree, int num, int pos)
{
	int offset = sizeof(treenode) + degree * type.length() + degree * sizeof(int) + pos * sizeof(RecordPosition);
	BlockGuard bg(block_mgr, scheme, num);
	*bg.addr<RecordPosition>(offset) = record_pos;
}
Value IndexManager::get_value(string& scheme, Type type, int num, int degree, int pos)
{
	BlockGuard bg(block_mgr, scheme, num);
	int offset = sizeof(treenode) + pos * type.length();
	Value temp; 
	temp.parse(bg.addr<void>(offset),type);
	return temp;

}
void IndexManager::insert_block(string& scheme, Type type, int block_index, int degree, int num, int pos)
{
	int offset = sizeof(treenode) + degree * type.length() + pos * sizeof(int);
	BlockGuard bg(block_mgr, scheme, num);
	block_index = *bg.addr<int>(offset);
}
void IndexManager::shift_value(string& scheme, Type type, int degree, int num, int pos, int direction)
{
	Value temp_value;
	if (direction > 0)for (int i = degree - 2; i >= pos; i--)
	{	
		temp_value = get_value(scheme, type, num, degree, i);
		insert_value(scheme, type, temp_value, degree,num, i + 1);
	}
	else
	{
		for (int i = pos; i < degree - 1; i++)
		{
			temp_value = get_value(scheme, type, num, degree, i);
			insert_value(scheme, type, temp_value, degree, num, i - 1);
		}
	}
}
RecordPosition IndexManager::get_record(string& scheme, Type type, int num, int degree, int pos)
{
	BlockGuard bg(block_mgr, scheme, num);
	int offset = sizeof(treenode) + degree * type.length() + degree*sizeof(int)+ sizeof(RecordPosition)* pos;
	RecordPosition temp_record = *bg.addr<RecordPosition>(offset);
	return temp_record;
}
void IndexManager::shift_record(string& scheme, Type type, int degree, int num, int pos, int direction)
{
	RecordPosition temp_record;
	if (direction > 0)for (int i = degree - 2; i >= pos; i--)
	{
		temp_record = get_record(scheme, type, num, degree, i);
		insert_record(scheme, type, temp_record, degree, num, i + 1);
	}
	else
	{
		for (int i = pos; i < degree - 1; i++)
		{
			temp_record = get_record(scheme, type, num, degree, i);
			insert_record(scheme, type, temp_record, degree, num, i - 1);
		}
	}
}
void IndexManager::shift_block(string& scheme, Type type, int degree, int num, int pos, int direction)
{
	int temp_block;
	if (direction > 0)for (int i = degree - 2; i >= pos; i--)
	{
		temp_block = get_block(scheme, type, num, degree, i);
		insert_block(scheme, type, temp_block, degree, num, i + 1);
	}
	else
	{
		for (int i = pos; i < degree - 1; i++)
		{
			temp_block = get_block(scheme, type, num,degree,i);
			insert_block(scheme, type, temp_block, degree, num, i - 1);
		}
	}
}
int IndexManager::get_block(string& scheme, Type type, int num, int degree, int pos)
{
	BlockGuard bg(block_mgr, scheme, num);
	int offset = sizeof(treenode) + degree * type.length() + sizeof(int)* pos;
	int index = *bg.addr<int>(offset);
	return index;
}
Value IndexManager::find_min(string& scheme, Type type, int num, int degree)
{
	BlockGuard bg(block_mgr, scheme, num);
	treenode* tree_info = bg.addr<treenode>();
	if (tree_info->Is_leaf == true)
	{
		Value temp_value = get_value(scheme, type, num, degree, 0);
		return temp_value;
	}
	else
	{
		int block_index = get_block(scheme, type, num, degree, 0);
		Value temp_value =find_min(scheme, type, block_index, degree);
		return temp_value;
	}
}
void IndexManager::work(string& scheme, const Relation& rel, int field_index, const Value& value, RecordPosition record_pos, int num, int degree)
{
	Type type = rel.fields[field_index].type;
	BlockGuard bg(block_mgr, scheme, num);
	treenode* tree_info = bg.addr<treenode>();
	// 如果不是叶子节点
	if (!tree_info->Is_leaf)
	{
		int size = tree_info->size;
		Value temp_value;
		int i;
		for (i = 0; i < size; i++)
		{
			temp_value = get_value(scheme, type, num, degree, i);
			if (temp_value.greater_than(value, type))
				break;
		}
		int block_index = get_block(scheme, type, num, degree, i);
		work(scheme, rel, field_index, value, record_pos, block_index, degree);
		if (tree_info->size == degree)
		{
			int new_num = get_new_block(scheme);
			BlockGuard new_bg(block_mgr, scheme, num);
			treenode* new_tree = new_bg.addr<treenode>();
			new_tree->fa = tree_info->fa;
			new_tree->Is_leaf = tree_info->Is_leaf;
			new_tree->Is_root = new_tree->Is_root;
			new_tree->size = degree - degree / 2;
			int temp_block;
			for (int i = degree/2; i < degree; i++)
			{
				if (i != 0)
				{
					temp_value = get_value(scheme, type, num, degree, i);
					insert_value(scheme, type, temp_value, degree, new_num, i - degree / 2 - 1);
				}
				temp_block = get_block(scheme, type, num, degree, i);
				insert_block(scheme, type, temp_block, degree,new_num, i - degree/2);
			}
			new_bg.set_modified();
			if (tree_info->Is_root == false)
			{
				Value min_value = find_min(scheme, type, new_num,degree);
				BlockGuard fa_bg(block_mgr, scheme, tree_info->fa);
				struct treenode* fa_head = fa_bg.addr<treenode>();
				
				for (i = 0; i < fa_head->size; i++)
				{
					temp_value = get_value(scheme, type, tree_info->fa, degree, i);
					if (temp_value.greater_than(min_value, type))	break;
				}
				shift_value(scheme, type, degree, tree_info->fa, i, 1);
				insert_value(scheme, type, min_value, degree, tree_info->fa, i);
				shift_block(scheme, type, degree, tree_info->fa, i + 1, 1);
				insert_block(scheme, type, new_num, degree, tree_info->fa, i + 1);
				fa_head->size++;

			}
			else
			{
				int new_root = get_new_block(scheme);
				tree_info->Is_root = false;
				new_tree->Is_root = false;
				tree_info->fa = new_root;
				new_tree->fa = new_root;
				BlockGuard root_bg(block_mgr, scheme, num);
				treenode* root_info = root_bg.addr<treenode>();
				root_info->fa = 0;
				root_info->Is_leaf = false;
				root_info->Is_root = true;
				root_info->next = 0;
				root_info->size = 1;
				Value new_value = get_value(scheme, type, new_num, degree, 0);
				insert_value(scheme, type, value, degree, new_num, 0);
				insert_block(scheme, type, num, degree, new_root, 0);
				insert_block(scheme, type, new_num, degree, new_root, 1);
				root_bg.set_modified();
			}
		}
		else
			bg.set_modified();
		return;
	}
	// 如果是叶子节点
	else
	{
		int size = tree_info->size;
		Value temp_value;
		int i;
		for (i = 0; i < size; i++)
		{
			temp_value = get_value(scheme, type, num, degree, i);
			if (temp_value.greater_than(value, type))
				break;
		}
		shift_value(scheme, type, degree, num, i, 1);
		insert_value(scheme, type, value, degree, num, i);
		shift_record(scheme, type, degree, num, i, 1);
		insert_record(scheme, type, record_pos, degree, num, i);
		bg.set_modified();
		size++;
		tree_info->size++;
		// 叶子节点已满
		if (tree_info->size == degree)
		{
			int new_num = get_new_block(scheme);
			BlockGuard new_bg(block_mgr, scheme, num);
			treenode* new_tree = new_bg.addr<treenode>();
			new_tree->fa = tree_info->fa;
			new_tree->Is_leaf = tree_info->Is_leaf;
			new_tree->Is_root = new_tree->Is_root;
			new_tree->next = tree_info->next;
			tree_info->next = new_num;
			new_tree->size = degree - degree / 2;
			RecordPosition temp_record;
			for (int i = degree / 2; i < tree_info->size; i++)
			{
				temp_value = get_value(scheme, type, num, degree, i);
				insert_value(scheme, type, value, degree, new_num, i - degree / 2);
				temp_record = get_record(scheme, type, new_num, degree, i);
				insert_record(scheme, type, record_pos, degree, new_num, i - degree / 2);
			}
			tree_info->size = degree / 2;
			new_bg.set_modified();
			// 该叶子节点不是根节点
			if (tree_info->Is_root == false) 
			{
				BlockGuard fa_bg(block_mgr, scheme, tree_info->fa);
				struct treenode* fa_head = fa_bg.addr<treenode>();
				Value new_value;
				new_value = get_value(scheme, type, new_num, degree, 0);
				for (i = 0; i < fa_head->size; i++)
				{
					temp_value = get_value(scheme, type, tree_info->fa, degree, i);
					if (temp_value.greater_than(new_value, type))	break;
				}
				shift_value(scheme, type, degree, tree_info->fa, i, 1);
				insert_value(scheme, type, new_value, degree, tree_info->fa, i);
				shift_block(scheme, type, degree, tree_info->fa, i + 1, 1);
				insert_block(scheme, type, new_num,degree,tree_info->fa,i+1);
				fa_head->size++;
			}
			// 叶子节点同时是根节点
			else
			{
				int new_root = get_new_block(scheme);
				tree_info->Is_root = false;
				new_tree->Is_root = false;
				tree_info->fa = new_root;
				new_tree->fa = new_root;
				BlockGuard root_bg(block_mgr, scheme, num);
				treenode* root_info = root_bg.addr<treenode>();
				root_info->fa = 0;
				root_info->Is_leaf = false;
				root_info->Is_root = true;
				root_info->next = 0;
				root_info->size = 1;
				Value new_value = get_value(scheme, type, new_num, degree, 0);
				insert_value(scheme, type, value, degree, new_num, 0);
				insert_block(scheme, type, num, degree, new_root, 0);
				insert_block(scheme, type, new_num, degree, new_root, 1);
				root_bg.set_modified();
			}
		}
		
	}
}
void IndexManager::add_item(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos)
{
	string& scheme = Files::index(rel.name, field_index);
	tree_information tree_info = get_information(rel, field_index, scheme);
	// degree = {Is_root + Is_leaf + fa + size} + data[degree] + block_pos[degree] + rec_address[degree]
	int degree = tree_info.degree;
	if (tree_info.root == -1)
	{
		int num = get_new_block(scheme);
		BlockGuard bg(block_mgr, scheme, num);
		tree_info.root = 1;
		struct treenode* new_node = bg.addr<struct treenode>();
		new_node->fa = 0;
		new_node->next = 0;
		new_node->Is_leaf = true;
		new_node->Is_root = true;
		new_node->size = 1;
		Type type = rel.fields[field_index].type;
		insert_value(scheme, type, value, degree, 1, 0);
		insert_record(scheme, type, record_pos, degree, 1, 0);
		bg.set_modified();
		return;
	}
	else
		work(scheme, rel, field_index, value, record_pos, tree_info.root, degree);
}
Nullable<RecordPosition> IndexManager::find(string& scheme, Type type, int degree, Value value, int num)
{
	BlockGuard bg(block_mgr, scheme, num);
	treenode tree_info = *bg.addr<treenode>();
	if (tree_info.Is_leaf == true)
	{
		int i;
		int size = tree_info.size;
		Value temp_value;
		for (i = 0; i < size; i++)
		{
			temp_value = get_value(scheme, type, num, degree, i);
			if (Value::cmp(type,value,temp_value) == 0) break;
		}
		if (i == size)	return Null();
		else return get_record(scheme, type, num, degree, i);
	}
	else
	{
		int i;
		int size = tree_info.size;
		Value temp_value;
		for (i = 0; i < size; i++)
		{
			temp_value = get_value(scheme, type, num, degree, i);
			if (Value::cmp(type, value, temp_value) == 0) break;
		}
		return find(scheme, type, degree, value, i);
	}
}