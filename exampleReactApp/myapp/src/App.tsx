import React, { useEffect, useState } from 'react';
import logo from './logo.svg';
import './App.css';


type todoModel = {
	id: number;
	name: string;
	isdone: boolean;
	created?: any;
}


const NewTask: React.FC<
	todoModel & 
	{
		update: (d: Partial<todoModel>) => void, 
		del?: () => void
	}
	> = 
	({id, name, del, isdone, update}) => 
{
	const onCheck = () => {
		update({isdone: !isdone});
	}
	const onWrite = (e: any) => {
		update({name: e.target.text});
	}
	return (
		<tr>
			<td>
				<span 
					className="pad-right"
				>
					{id || 0} 
				</span>
			</td>
			<td>
				<input 
					className="pad-right" 
					type="text" 
					defaultValue={name} 
					onChange={(e) => onWrite(e)} 
				/>
			</td>
			<td>
				<input  
					className="pad-right"
					type="checkbox" 
					defaultChecked={isdone} 
					onClick={() => onCheck()}
				/>
			</td>
			{del && (
				<td>
					<button
						onClick={() => del()}
					>x
					</button>
				</td>

			)}
		</tr>
	)
}
const Task: React.FC<
	{task: todoModel} &
	{
		update: (d: Partial<todoModel>) => void, 
		del?: () => void
	}
	> = 
	({task, del, update}) => 
{
	const [ internal, setInternal ] = useState<todoModel>(task);
	const onCheck = () => {
		setInternal(v => ({...v, isdone: !v.isdone}));
		//update({isdone: !isdone})
	}
	const onWrite = (e: any) => {
		setInternal(v => ({...v, name: e.target.value}));
		//update({name: e.target.text})
	}
	const isChanged = JSON.stringify(internal) !== JSON.stringify(task);
	return (
		<tr>
			<td>
				<span 
					className="pad-right"
				>
					{internal.id} 
				</span>
			</td>
			<td>
				<input 
					className="pad-right" 
					type="text" 
					defaultValue={internal.name} 
					onChange={(e) => onWrite(e)} 
				/>
			</td>
			<td>
				<input  
					className="pad-right"
					type="checkbox" 
					defaultChecked={internal.isdone} 
					onClick={() => onCheck()}
				/>
			</td>
			{del && (
				<td>
					<button
						onClick={() => del()}
					>x
					</button>
				</td>
			)}
			{isChanged && (
				<td>
					<button
						onClick={() => update(internal)}
					>commit
					</button>
				</td>
			)}
		</tr>
	)
}


const defaultData: todoModel = {
	id: 0, name: "new task", isdone: false
}

const getMax = (t: number[]) => {
	if(t.length == 0) {
		return 0;
	}
	let max = t[0];
	for(let i = 1; i < t.length; i++) {
		if(t[i] > max) {
			max = t[i];
		}
	}
	return max as number;
}

function App() {
	const [data, setData] = useState<todoModel[]>([]);
	const nextIndex = getMax(data.map(v => v.id))+1

	const [newItem, setItem] = useState<todoModel>(defaultData);
	const updateData = () => {
		fetch("http://kczuba.ddns.net/gettasks")
			.then(d => d.json())
			.then(d => setData(d))
			.catch(e => console.log(e));
	}
	useEffect(() => {
		updateData();
	}, [])

	const handleArrayUpdate = (pageIndex: number, newData: Partial<todoModel>) => {
		setData(v => 
			v.map((item, arrayIndex) => 
				pageIndex === arrayIndex ? {...item, ...newData} : item
			)
		)
	};
	const updateItem = (id: number, d: todoModel) => {
		console.log(d);
		fetch(`http://kczuba.ddns.net/updatetask?id=${id}`, {
			method: 'PUT',
			body: `name=${d.name}&isdone=${d.isdone}`
		})
			.then(() => updateData())
			.catch(e => console.log(e));
	}
	const deleteItem = (itemId: number) => {
		fetch(`http://kczuba.ddns.net/deltask?id=${itemId}`, {method: 'DELETE'})
			.then(() => updateData())
			.catch(e => console.log(e));
	};
	const createItem = () => {
		if(newItem.name) {
			fetch(`http://kczuba.ddns.net/createtask`, {
					method: 'POST',
					body: `id=${nextIndex}&name=${newItem.name}&isdone=${newItem.isdone}`
				})
				.then(() => updateData())
				.catch(e => console.log(e));
		}
	};
	console.log(data);
  return (
		<div className="container">
			<div className={"add"}>
				<div className="pad-bot"> {}DODAJ NOWE ZADANIE: </div>
			<table>
				<NewTask 
					id={nextIndex}
					name={newItem.name}
					isdone={newItem.isdone}
					created={newItem.created}
					update={
						(newData: Partial<todoModel>) =>
							setItem(v => ({...v, ...newData}))
					}
				/>
			</table>
			<div
				className={"center"}
			>
				<button 
					onClick={() => createItem()}
				> 
					Create 
				</button>
			</div>
			<hr/>
			<table>
				{data.sort((i, j) => i.id-j.id).map((d, i) => (
				<Task 
					task={d} 
					key={d.id}
					del={() => deleteItem(d.id)}
					update={
						(newData: Partial<todoModel>) => 
							updateItem(d.id, {...d, ...newData})
					}
				/>
			))}
			</table>
			</div>
			<div className="add">
				<hr/>
				<div className="center">
					<button> Commit </button>
				</div>
			</div>
		</div>
  );
}

export default App;
