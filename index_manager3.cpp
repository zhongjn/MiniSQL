#include "index_manager.h"
IndexManager::IndexManager(BlockManager* block_mgr):block_mgr(block_mgr )
{
}
struct tree_information 
{
	int root;
	int degree;
};
struct treenode
{
	bool Is_root;
	bool Is_leaf;
	int size;
	int fa;
	int next;
};
/*void IndexManager::add_index(const Relation& rel, int field_index, unique_ptr<Scanner> scanner)
{
	vector<Value> value_set;
	vector<RecordPosition> pos_set;
	vector<Value>::iterator vi;
	vector<RecordPosition>::iterator pi;
	Type temp_type = rel.fields[field_index].type;
	while (scanner->next())
	{
		Record& rec = scanner->current();
		Value temp = rec.values[field_index];
		vi = value_set.begin();
		pi = pos_set.begin();
		while (vi < value_set.end() && temp.cmp(*vi, temp_type) == 1)
		{	
			vi++;
			pi++;
		}
		value_set.insert(vi, temp);
		pos_set.insert(pi,rec.physical_position);
	}
	int degree = (BLOCK_SIZE-(sizeof(bool)*2+sizeof(int)))/(rel.fields[field_index].type.length()+sizeof(int)+sizeof(RecordPosition));
	string& scheme = Files::index(rel.name, field_index);
	if (block_mgr->file_blocks(scheme) == 0) 
	{
		block_mgr->file_append_block(scheme);
	}
		BlockGuard bg_init(block_mgr, scheme, 0);
		int offset = 0;
		struct tree_information* head = new(bg_init.addr()) struct tree_information;
		head->degree = degree;
		head->root = -1;
		bg_init.set_modified();
		int size = value_set.size();
		int num = 1;
		int tot = 0;
		vi = value_set.begin();
		pi = pos_set.begin();
		vector<Value> quene_v;
		vector<int> quene_c;
		vector<int>::iterator ci;
		while (tot < size)
		{
			block_mgr->file_append_block(scheme);
			BlockGuard bg(block_mgr, scheme, num);
			offset = 0;
			struct treenode* node = new(bg.addr()) struct treenode;
			node->Is_leaf = true;
			node->Is_root = false;
			node->size = 0;
			offset += sizeof(struct treenode);
			if (vi < value_set.end())
			{
				quene_v.push_back(*vi);
				quene_c.push_back(num);
			}
			for (;vi < value_set.end(); vi++)
			{
				(node->size)++;
				(*vi).write(bg.addr<void>(offset), temp_type);
				offset = offset + rel.fields[field_index].type.length();
				if (node->size == degree - 1) break;
			}
			offset = sizeof(struct treenode) + (degree - 1)*rel.fields[field_index].type.length() + degree*sizeof(int);
			int count = 0;
			for (;pi < pos_set.end(); pi++)
			{
				count++;
				void* addr = (void*)((uint8_t*)bg.addr<void>() + offset);
				new(addr) RecordPosition(*pi);
				offset = offset + sizeof(RecordPosition);
				if (count == node->size)	break;
			}
			bg.set_modified();
			tot = tot + node->size;
			num = num + 1;
		}
		if (size < degree) return;
		int old_size = size, new_size = 0;
		while (old_size > 1)
		{
			new_size = 0;
			tot = 0;
			int i = 0;
			int j = 0;
			int upbound = degree;
			while (tot < old_size)
			{
				if (old_size - tot >= degree + degree / 2 + 1)
					upbound = degree;
				else if (old_size - tot <= degree)
					upbound = old_size - tot;
				else upbound = degree / 2 + 1;
				num++;
				Value begin_v = quene_v[i];
				int	  begin_c = quene_c[j];
				block_mgr->file_append_block(scheme);
				BlockGuard bg(block_mgr, scheme, num);
				struct treenode* node = new(bg.addr()) struct treenode;
				node->Is_leaf = false;
				node->Is_root = false;
				node->size = 0;
				offset += sizeof(struct treenode);
				for (; i < old_size; i++)
				{
					(node->size)++;
					if (node->size != 1)
					{
						quene_v[i].write(bg.addr<void>(offset), temp_type);
						offset = offset + rel.fields[field_index].type.length();
					}
					if (node->size == upbound) break;
				}
				offset = sizeof(struct treenode) + (upbound)*rel.fields[field_index].type.length();
				int count = 0;
				for (; j < i; j++)
				{
					void* addr = (void*)((uint8_t*)bg.addr<void>() + offset);
					new(addr) int(quene_c[j]);
					offset = offset + sizeof(int);
				}
				bg_init.set_modified();
				quene_v[new_size] = move(begin_v);
				quene_c[new_size] = begin_c;
				new_size++;
			}
			old_size = new_size;
		}
		BlockGuard bg(block_mgr, scheme, num);
		struct treenode* node = (struct treenode*)bg.addr<void>();
		node->Is_root = true;
		bg = BlockGuard(block_mgr, scheme, 1);
		head = (struct tree_information*)(bg_init.addr<void>());
		head->root = num;
}*/
void IndexManager::work(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos, int num, int degree)
{
	string& scheme = Files::index(rel.name, field_index);
	int value_size = rel.fields[field_index].type.length();
	Type value_type = rel.fields[field_index].type;
	BlockGuard bg(block_mgr, scheme, num);
	struct treenode* head = (struct treenode*)bg.addr<void>(0);
	// 如果找到了叶节点
	if (head->Is_leaf)
	{
		int size = head->size, index = 0;
		Value temp_value;
		int offset = sizeof(treenode);
		// 查询插入位置
		while (index < size)
		{
			temp_value.parse(bg.addr<void>(offset), value_type);
			if (temp_value.greater_than(value, value_type)) break;
			offset = offset + value_size;
			index++;
		}
		// 如果插入位置在最后
		if (index == size)
		{
			value.write(bg.addr<void>(offset), value_type);
			new(bg.addr<void>(sizeof(struct treenode) + value_size * degree + sizeof(int)*degree + sizeof(RecordPosition) * size)) RecordPosition(record_pos);
			size++;
			head->size++;
			bg.set_modified();
		}
		// 否则在中间进行插入
		else
		{
			for (int i = size - 1; i >= index; i++)
			{
				offset = sizeof(struct treenode);
				temp_value.parse(bg.addr<void>(offset + i * value_size), value_type);
				temp_value.write(bg.addr<void>(offset + (1 + i)*value_size), value_type);
				*bg.addr<RecordPosition>(sizeof(struct treenode) + value_size * degree + sizeof(int)*degree + sizeof(RecordPosition) * (i + 1))
					= *bg.addr<RecordPosition>(sizeof(struct treenode) + value_size * degree + sizeof(int)*degree + sizeof(RecordPosition) * (i));
			}
			value.write(bg.addr<void>(offset + index * offset), value_type);
		}
		if (head->size == degree)
		{
			block_mgr->file_append_block(scheme);
			int new_block = block_mgr->file_blocks(scheme) - 1;
			BlockGuard new_bg(block_mgr, scheme, new_block);
			struct treenode* new_head = (struct treenode*)new_bg.addr<void>(0);
			int new_size = head->size - degree / 2;
			new_head->fa = head->fa;
			new_head->Is_leaf = head->Is_leaf;
			new_head->Is_root = head->Is_root;
			new_head->size = new_size;
			for (int i = degree / 2; i < head->size; i++)
			{
				temp_value.parse(bg.addr<void>(sizeof(struct treenode) + value_size * i), value_type);
				temp_value.write(new_bg.addr<void>(sizeof(struct treenode) + value_size * (i - degree / 2)), value_type);
				*new_bg.addr<RecordPosition>(sizeof(struct treenode) + value_size * degree + sizeof(int)*degree + sizeof(RecordPosition) * (i - degree / 2))
					= *bg.addr<RecordPosition>(sizeof(struct treenode) + value_size * degree + sizeof(int)*degree + sizeof(RecordPosition) * (i));
			}
			head->size = degree / 2;
			new_bg.set_modified();
			BlockGuard fa_bg(block_mgr, scheme, head->fa);
			struct treenode* fa_head = (struct treenode*)fa_bg.addr<void>(0);
			fa_head->size++;
			Value new_value;
			new_value.parse(new_bg.addr<void>(sizeof(struct treenode)), value_type);
			int index = 0;
			offset = sizeof(struct treenode);
			size = fa_head->size;
			while (index < size)
			{
				temp_value.parse(bg.addr<void>(offset), value_type);
				if (temp_value.greater_than(new_value, value_type)) break;
				offset = offset + value_size;
				index++;
			}
			if (index == size)
			{
				new_value.write(fa_bg.addr<void>(offset), value_type);
				new(fa_bg.addr<void>(sizeof(struct treenode) + value_size * degree + sizeof(int)*(size + 1))) int(new_block);
				size++;
				fa_head->size = size;

			}
			// 否则在中间进行插入
			else
			{
				for (int i = size - 1; i >= index; i++)
				{
					offset = sizeof(struct treenode);
					temp_value.parse(fa_bg.addr<void>(offset + i * value_size), value_type);
					temp_value.write(fa_bg.addr<void>(offset + (1 + i)*value_size), value_type);
					*fa_bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int) * (i + 2))
						= *fa_bg.addr<int>(sizeof(struct treenode) + sizeof(int)*degree + sizeof(int) * (i + 1));
				}
				value.write(bg.addr<void>(offset + index * offset), value_type);
				*fa_bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int)*(index + 1));
			}
			fa_bg.set_modified();
			fa_head->size++;
		}
		bg.set_modified();
		return;
	}
	// 如果不是叶节点
	else
	{
		int size = head->size;
		int i;
		Value temp_value;
		int offset = sizeof(struct treenode);
		for (i = 0; i < size; i++)
		{
			temp_value.parse(bg.addr<void>(offset + i * value_size), value_type);
			if (temp_value.greater_than(value, value_type))
				break;
		}
		int block_id = *(bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int)*i));
		work(rel, field_index, value, record_pos, block_id, degree);
		if (head->size == degree)
		{
			block_mgr->file_append_block(scheme);
			int new_block = block_mgr->file_blocks(scheme) - 1;
			BlockGuard new_bg(block_mgr, scheme, new_block);
			struct treenode* new_head = new_bg.addr<struct treenode>(0);
			int new_size = head->size - (degree + 1) / 2 + 1;
			new_head->fa = head->fa;
			new_head->Is_leaf = head->Is_leaf;
			new_head->Is_root = head->Is_root;
			new_head->size = new_size;
			for (int i = (degree + 1) / 2; i <= head->size; i++)
			{
				if (i != (degree + 1) / 2)
				{
					temp_value.parse(bg.addr<void>(sizeof(struct treenode) + value_size * (i - 1)), value_type);
					temp_value.write(new_bg.addr<void>(sizeof(struct treenode) + value_size * (i - (degree + 1) / 2 - 1)), value_type);
				}
				*new_bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int) * (i - (degree + 1) / 2))
					= *bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int) * (i));
			}
			head->size = (degree + 1) / 2 - 1;
			new_bg.set_modified();
			if (head->fa != -1)
			{
				BlockGuard fa_bg(block_mgr, scheme, head->fa);
				struct treenode* fa_head = (struct treenode*)fa_bg.addr<void>(0);
				fa_head->size++;
				Value new_value;
				new_value.parse(new_bg.addr<void>(sizeof(struct treenode)), value_type);
				int index = 0;
				offset = sizeof(struct treenode);
				size = fa_head->size;
				while (index < size)
				{
					temp_value.parse(bg.addr<void>(offset), value_type);
					if (temp_value.greater_than(new_value, value_type)) break;
					offset = offset + value_size;
					index++;
				}
				if (index == size)
				{
					new_value.write(fa_bg.addr<void>(offset), value_type);
					new(fa_bg.addr<void>(sizeof(struct treenode) + value_size * degree + sizeof(int)*(size + 1))) int(new_block);
					size++;
					fa_head->size = size;

				}
				// 否则在中间进行插入
				else
				{
					for (int i = size - 1; i >= index; i++)
					{
						offset = sizeof(struct treenode);
						temp_value.parse(fa_bg.addr<void>(offset + i * value_size), value_type);
						temp_value.write(fa_bg.addr<void>(offset + (1 + i)*value_size), value_type);
						*fa_bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int) * (i + 2))
							= *fa_bg.addr<int>(sizeof(struct treenode) + sizeof(int)*degree + sizeof(int) * (i + 1));
					}
					value.write(bg.addr<void>(offset + index * offset), value_type);
					*fa_bg.addr<int>(sizeof(struct treenode) + value_size * degree + sizeof(int)*(index + 1));
				}
				fa_bg.set_modified();
				fa_head->size++;
			}
			else
			{
				block_mgr->file_append_block(scheme);
				int root_block = block_mgr->file_blocks(scheme) - 1;
				BlockGuard root_bg(block_mgr, scheme, root_block);
				head->fa = root_block;
				new_head->fa = root_block;
				head->Is_root = false;
				new_head->Is_root = false;
				struct treenode* root_head = root_bg.addr<struct treenode>();
				root_head->fa = -1;
				root_head->Is_leaf = false;
				root_head->Is_root = true;
				root_head->size = 1;
				BlockGuard bg_init(block_mgr, scheme, 0);
				tree_information* head = bg_init.addr<struct tree_information>();
				head->root = root_block;
				Value root_value = find()
			}
			bg.set_modified();
			return;
		}
	}
}

void IndexManager::add_item(const Relation& rel, int field_index, const Value& value, RecordPosition record_pos)
{
	string& scheme = Files::index(rel.name, field_index);
	struct tree_information* head;
	if (block_mgr->file_blocks(scheme) == 0)
	{
		block_mgr->file_append_block(scheme);
		BlockGuard bg_init(block_mgr, scheme, 0);
		head = new(bg_init.addr()) struct tree_information;
		// degree = {Is_root + Is_leaf + fa + size} + data[degree] + block_pos[degree] + rec_address[degree]
		int degree = (BLOCK_SIZE - (sizeof(bool) * 2 + 2 * sizeof(int))) / (rel.fields[field_index].type.length() + sizeof(int) + sizeof(RecordPosition));
		head->degree = degree;
		head->root = -1;
		bg_init.set_modified();
	}
	BlockGuard bg(block_mgr, scheme, 0);
	head = (struct tree_information*)bg.addr<void>();
	int degree = head->degree;
	if (head->root = -1)
	{
		int num = 1;
		block_mgr->file_append_block(scheme);
		BlockGuard bg(block_mgr, scheme, 1);
		head->root = 1;
		struct treenode* new_Node = new(bg.addr<void>()) struct treenode;
		new_Node->fa = 0;
		new_Node->Is_leaf = true;
		new_Node->Is_root = true;
		new_Node->size = 1;
		int offset = sizeof(struct treenode);
		value.write(bg.addr<void>(offset), rel.fields[field_index].type);
		offset = offset + rel.fields[field_index].type.length()*degree + sizeof(int)*degree;
		new(bg.addr<void>(offset)) RecordPosition(record_pos);
		bg.set_modified();
		return;
	}
	else
	{
		work(rel,field_index, value, record_pos,head->root,head->degree);
	}
}
