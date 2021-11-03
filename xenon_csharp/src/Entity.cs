﻿using System;
using System.Runtime.CompilerServices;

namespace Xenon {
	public class Entity {

		public ulong id { get; private set; }

		protected Entity() { id = 0; }
		internal Entity(ulong id) {
			this.id = id;
		}

		public Vector3 position {
			get {
				return GetComponent<TransformComponent>().Position;
			}
			set {
				GetComponent<TransformComponent>().Position = value;
			}
		}

		public Vector3 rotation {
			get {
				return GetComponent<TransformComponent>().Rotation;
			}
			set {
				GetComponent<TransformComponent>().Rotation = value;
			}
		}

		public Vector3 scale {
			get {
				return GetComponent<TransformComponent>().Scale;
			}
			set {
				GetComponent<TransformComponent>().Scale = value;
			}
		}

		public T CreateComponent<T>() where T : Component, new() {
			CreateComponent_Native(id, typeof(T));
			T component = new T();
			component.Entity = this;
			return component;
		}

		public bool HasComponent<T>() where T : Component, new() {
			return HasComponent_Native(id, typeof(T));
		}

		public T GetComponent<T>() where T : Component, new() {
			if (HasComponent<T>()) {
				T component = new T();
				component.Entity = this;
				return component;
			}
			return null;
		}

		public Entity FindEntityByTag(string tag) {
			ulong entityID = FindEntityByTag_Native(tag);
			return new Entity(entityID);
		}

		public Entity FindEntityByID(ulong entityID) {
			// TODO: Verify the entity id
			return new Entity(entityID);
		}


		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern void CreateComponent_Native(ulong entityID, Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern bool HasComponent_Native(ulong entityID, Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
		private static extern ulong FindEntityByTag_Native(string tag);
	}
}
