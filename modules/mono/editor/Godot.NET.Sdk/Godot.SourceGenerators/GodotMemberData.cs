using System.Collections.Immutable;
using Microsoft.CodeAnalysis;

namespace Godot.SourceGenerators
{
    public struct GodotMethodData
    {
        public GodotMethodData(IMethodSymbol method, ImmutableArray<MarshalType> paramTypes,
            ImmutableArray<ITypeSymbol> paramTypeSymbols, MarshalType? retType, ITypeSymbol? retSymbol)
        {
            Method = method;
            ParamTypes = paramTypes;
            ParamTypeSymbols = paramTypeSymbols;
            RetType = retType;
            RetSymbol = retSymbol;
        }

        public IMethodSymbol Method { get; }
        public ImmutableArray<MarshalType> ParamTypes { get; }
        public ImmutableArray<ITypeSymbol> ParamTypeSymbols { get; }
        public MarshalType? RetType { get; }
        public ITypeSymbol? RetSymbol { get; }
    }

    public struct GodotSignalDelegateData
    {
        public GodotSignalDelegateData(string name, INamedTypeSymbol delegateSymbol, GodotMethodData invokeMethodData)
        {
            Name = name;
            DelegateSymbol = delegateSymbol;
            InvokeMethodData = invokeMethodData;
        }

        public string Name { get; }
        public INamedTypeSymbol DelegateSymbol { get; }
        public GodotMethodData InvokeMethodData { get; }
    }

    public struct GodotPropertyData
    {
        public GodotPropertyData(IPropertySymbol propertySymbol, MarshalType type)
        {
            PropertySymbol = propertySymbol;
            Type = type;
        }

        public IPropertySymbol PropertySymbol { get; }
        public MarshalType Type { get; }
    }

    public struct GodotFieldData
    {
        public GodotFieldData(IFieldSymbol fieldSymbol, MarshalType type)
        {
            FieldSymbol = fieldSymbol;
            Type = type;
        }

        public IFieldSymbol FieldSymbol { get; }
        public MarshalType Type { get; }
    }

    public struct GodotPropertyOrFieldData
    {
        public GodotPropertyOrFieldData(ISymbol symbol, MarshalType type)
        {
            Symbol = symbol;
            Type = type;
        }

        public GodotPropertyOrFieldData(GodotPropertyData propertyData)
            : this(propertyData.PropertySymbol, propertyData.Type)
        {
        }

        public GodotPropertyOrFieldData(GodotFieldData fieldData)
            : this(fieldData.FieldSymbol, fieldData.Type)
        {
        }

        public ISymbol Symbol { get; }
        public MarshalType Type { get; }
    }
}
